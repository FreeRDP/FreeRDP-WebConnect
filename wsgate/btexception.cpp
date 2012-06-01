/* vim: set et ts=4 sw=4 cindent:
 *
 * FreeRDP-WebConnect,
 * A gateway for seamless access to your RDP-Sessions in any HTML5-compliant browser.
 *
 * Copyright 2012 Fritz Elfert <wsgate@fritz-elfert.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "btexception.hpp"
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <pthread.h>

#ifdef HAVE_EXECINFO_H
# include <execinfo.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


extern "C" {
#define DMGL_PARAMS      (1 << 0)       /* Include function args */
#define DMGL_ANSI        (1 << 1)       /* Include const, volatile, etc */
#define DMGL_JAVA        (1 << 2)       /* Demangle as Java rather than C++. */
#define DMGL_VERBOSE     (1 << 3)       /* Include implementation details.  */
#define DMGL_TYPES       (1 << 4)       /* Also try to demangle type encodings.  */
#define DMGL_RET_POSTFIX (1 << 5)       /* Print function return types (when
                                           present) after function signature */
    extern char *cplus_demangle(const char *, int);
}

static pthread_mutex_t trace_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef USE_BFD
# include <bfd.h>

static asymbol **syms;
static bfd *abfd;

static void fetchsyms (bfd *abfd)
{
    long symcount;
    unsigned int size;
    if (0 == (bfd_get_file_flags(abfd) & HAS_SYMS))
        return;
    symcount = bfd_read_minisymbols (abfd,
            FALSE, reinterpret_cast<void **>(&syms), &size);
    if (0 == symcount)
        symcount = bfd_read_minisymbols (abfd,
                TRUE /* dynamic */, reinterpret_cast<void **>(&syms), &size);
}

typedef struct {
    bfd_vma pc;
    const char *filename;
    const char *functionname;
    unsigned int line;
    bfd_boolean found;
} bfdstate_t;

static void findit(bfd *abfd, asection *section, void *data)
{
    bfdstate_t *state = (bfdstate_t *)data;
    bfd_vma vma;
    bfd_size_type size;

    if (state->found)
        return;
    if (0 == (bfd_get_section_flags(abfd, section) & SEC_ALLOC))
        return;
    vma = bfd_get_section_vma (abfd, section);
    if (state->pc < vma)
        return;
    size = bfd_get_section_size (section);
    if (state->pc >= vma + size)
        return;
    state->found =
        bfd_find_nearest_line(abfd, section, syms, state->pc - vma,
                &state->filename, &state->functionname, &state->line);
}

static void exit_backtrace()
{
    if (NULL != syms)
        free (syms);
    if (NULL != abfd)
        bfd_close (abfd);
    abfd = NULL;
    syms = NULL;
}

static int init_backtrace()
{
    static int init = 1;
    static int sret = 0;
    char **matching;

    if (init) {
        init = 0;
        bfd_init();
        if (!bfd_set_default_target("i686-redhat-linux-gnu"))
            return 0;
        abfd = bfd_openr("/proc/self/exe", NULL);
        if (NULL == abfd)
            return 0;
        if (bfd_check_format(abfd, bfd_archive))
            return 0;
        if (!bfd_check_format_matches(abfd, bfd_object, &matching))
            return 0;
        fetchsyms(abfd);
        atexit(exit_backtrace);
        sret = 1;
    }
    return sret;
}

#endif // USE_BFD

#ifdef USE_DWARF
# include <inttypes.h>
# include <libdwfl.h>
# include <dwarf.h>

static char *debuginfo_path = NULL;
static Dwfl_Callbacks proc_callbacks;
static Dwfl *dwfl = NULL;

static void exit_backtrace ()
{
    if (dwfl)
        dwfl_end(dwfl);
}

static bool init_backtrace ()
{
    static int init = 1;
    static int sret = 0;
    if (init) {
        init = 0;
        proc_callbacks.find_debuginfo = dwfl_standard_find_debuginfo;
        proc_callbacks.debuginfo_path = &debuginfo_path;
        proc_callbacks.find_elf = dwfl_linux_proc_find_elf;
        dwfl = dwfl_begin(&proc_callbacks);
        sret = (0 == dwfl_linux_proc_report(dwfl, ::getpid()));
        atexit(exit_backtrace);
    }
    return sret;
}

static void cleanup_dwarf(Dwarf_Die *scopes)
{
    if (scopes)
        free(scopes);
}
static bool print_dwarf_function(Dwfl_Module *mod, Dwarf_Addr addr, std::ostringstream & oss)
{
    Dwarf_Addr bias = 0;
    Dwarf_Die *cudie = dwfl_module_addrdie (mod, addr, &bias);
    Dwarf_Die *scopes = NULL;
    int nscopes = dwarf_getscopes(cudie, addr - bias, &scopes);
    if (0 >= nscopes) {
        cleanup_dwarf(scopes);
        return false;
    }
    for (int i = 0; i < nscopes; ++i) {
        const char *name;
        Dwarf_Files *files;

        switch (dwarf_tag(&scopes[i])) {
            case DW_TAG_subprogram:
                if (NULL == (name = dwarf_diename(&scopes[i]))) {
                    cleanup_dwarf(scopes);
                    return false;
                }
                oss << name << " ";
                cleanup_dwarf(scopes);
                return true;
            case DW_TAG_inlined_subroutine:
                if (NULL == (name = dwarf_diename(&scopes[i]))) {
                    cleanup_dwarf(scopes);
                    return false;
                }
                oss << name << " inlined";
                if (0 == dwarf_getsrcfiles(cudie, &files, NULL)) {
                    Dwarf_Attribute attr_mem;
                    Dwarf_Word val;
                    if (0 == dwarf_formudata(dwarf_attr(&scopes[i], DW_AT_call_file, &attr_mem), &val)) {
                        const char *file = dwarf_filesrc(files, val, NULL, NULL);
                        unsigned int lnr = 0;
                        unsigned int col = 0;
                        if (0 == dwarf_formudata(dwarf_attr(&scopes[i],
                                        DW_AT_call_line, &attr_mem), &val)) {
                            lnr = val;
                        }
                        if (0 == dwarf_formudata(dwarf_attr(&scopes[i],
                                        DW_AT_call_column, &attr_mem), &val)) {
                            col = val;
                        }
                        if (NULL == file) {
                            file = "???";
                        }
                        if (0 == lnr) {
                            oss << " from " << file;
                        } else {
                            oss << " at " << file << ", line " << lnr;
                            if (0 != col) {
                                oss << ":" << col;
                            }
                        }
                    }
                }
                oss << " in ";
                continue;
        }
    }
    cleanup_dwarf(scopes);
    return false;
}
#endif // USE_DWARF

namespace tracing {

#ifdef USE_BFD
    bfd_tracer::bfd_tracer(const bfd_tracer &other) :
        maxframes(other.maxframes),
        frames(other.frames),
        tbuf(new void* [maxframes])
    {
        memcpy(tbuf, other.tbuf, sizeof(void *) * maxframes);
    }

    bfd_tracer::bfd_tracer(int _maxframes) :
        maxframes(_maxframes), frames(0), tbuf(new void* [_maxframes])
    {
        frames = backtrace(tbuf, maxframes);
    }

    bfd_tracer::~bfd_tracer()
    {
        delete [] tbuf;
    }

    const std::string & bfd_tracer::trace(int skip) const {
        static std::string ret;
        ret.clear();
        if (init_backtrace()) {
            for (int i = skip; i < frames ; i++) {
                std::ostringstream oss;
                bfdstate_t state;
                state.pc = reinterpret_cast<bfd_vma>(tbuf[i]);
                state.found = FALSE;
                bfd_map_over_sections(abfd, findit, &state);
                oss << std::setw(3) << i - skip << " "
                    << std::setfill('0') << std::setw(10)
                    << std::hex << std::internal
                    << tbuf[i] << " " << std::dec << std::setw(0);
                if (! state.found) {
                    oss << "[no debug info]" << std::endl;
                } else {
                    do {
                        const char *name;
                        char *alloc = NULL;

                        name = state.functionname;
                        if (NULL == name || '\0' == *name)
                            name = "??";
                        else {
#ifdef HAVE_BFD_DEMANGLE
                            alloc = bfd_demangle(abfd, name,
                                    DMGL_ANSI|DMGL_PARAMS|DMGL_TYPES);
#else
                            alloc = cplus_demangle(name,
                                    DMGL_ANSI|DMGL_PARAMS|DMGL_TYPES);
#endif
                            if (alloc != NULL)
                                name = alloc;
                        }

                        oss << name << " [";
                        if (alloc != NULL)
                            free (alloc);
                        if (state.filename) {
                            oss << state.filename << ", line " << state.line;
                        } else {
                            oss << "no debug info";
                        }
                        oss << "]" << std::endl;

                        state.found =
                            bfd_find_inliner_info(abfd,
                                    &state.filename, &state.functionname, &state.line);
                        if (state.found) {
                            oss << "    inlined at " << std::dec;
                        }
                    } while (state.found);
                }
                ret.append(oss.str());
            }
        } else {
            for (int i = skip; i < frames ; i++) {
                std::ostringstream oss;
                oss << std::setw(3) << i - skip << " "
                    << std::setfill('0') << std::setw(10) << std::hex << std::internal
                    << tbuf[i] << " [no debug info]" << std::endl;
                ret.append(oss.str());
            }
        }
        return ret;
    }
#endif // USE_BFD

#ifdef USE_DWARF
    dwarf_tracer::dwarf_tracer(const dwarf_tracer &other) :
        maxframes(other.maxframes),
        frames(other.frames),
        tbuf(new void* [maxframes])
    {
        memcpy(tbuf, other.tbuf, sizeof(void *) * maxframes);
    }

    dwarf_tracer::dwarf_tracer(int _maxframes) :
        maxframes(_maxframes), frames(0), tbuf(new void* [_maxframes])
    {
        frames = backtrace(tbuf, maxframes);
    }

    dwarf_tracer::~dwarf_tracer()
    {
        delete [] tbuf;
    }

    const std::string & dwarf_tracer::trace(int skip) const {
        static std::string ret;
        ret.clear();
        if (init_backtrace()) {
            for (int i = skip; i < frames ; i++) {
                std::ostringstream oss;
                uintmax_t addr = reinterpret_cast<uintmax_t>(tbuf[i]);
                Dwfl_Module *mod = dwfl_addrmodule(dwfl, addr);
                oss << std::setw(3) << i - skip << " "
                    << std::setfill('0') << std::setw(10)
                    << std::hex << std::internal
                    << tbuf[i] << " " << std::dec << std::setw(0);
                if (!print_dwarf_function(mod, addr, oss)) {
                    const char *mname = dwfl_module_addrname(mod, addr);
                    if (mname) {
#ifdef HAVE_LIBIBERTY
                        char *alloc = cplus_demangle(const_cast<char *>(mname), DMGL_ANSI|DMGL_PARAMS|DMGL_TYPES);
                        oss << (alloc ? alloc : mname) << " ";
                        if (alloc)
                            free(alloc);
#else
                        oss << mname << " ";
#endif
                    } else {
                        oss << "?? ";
                    }
                }
                Dwfl_Line *line = dwfl_module_getsrc(mod, addr);
                const char *src;
                int lineno, linecol;
                if (NULL != line &&
                        (src = dwfl_lineinfo(line, &addr, &lineno, &linecol, NULL, NULL)) != NULL) {
                    if (linecol != 0)
                        oss << src << ", line " << lineno << ":" << linecol << std::endl;
                    else
                        oss << src << ", line " << lineno << std::endl;
                } else
                    oss << "[no debug info]" << std::endl;
                ret.append(oss.str());
            }
        } else {
            for (int i = skip; i < frames; i++) {
                std::ostringstream oss;
                oss << std::setw(3) << i - skip << " "
                    << std::setfill('0') << std::setw(10) << std::hex << std::internal
                    << tbuf[i] << " [no debug info]" << std::endl;
                ret.append(oss.str());
            }
        }
        return ret;
    }
#endif // USE_DWARF

    exception::exception() throw() :
        std::exception(), tracer(100)
    {
    }

    exception::~exception() throw()
    {
    }

    const char* exception::where() const throw() {
        pthread_mutex_lock(&trace_mutex);
        const char *ret = tracer.trace(2).c_str();
        pthread_mutex_unlock(&trace_mutex);
        return ret;
    }
}
