#ifndef BTEXCEPTION_H
#define BTEXCEPTION_H

#include <exception>
#include <string>

namespace tracing {

    class dummy_tracer
    {
        public:
            dummy_tracer(int) { }

            std::string trace(int) const
            {
                return "Tracing disabled";
            }
    };

#ifdef USE_BFD
    class bfd_tracer
    {
        public:
            bfd_tracer(int _maxframes);

            ~bfd_tracer();

            const std::string & trace(int skip) const;

            bfd_tracer(const bfd_tracer &);

            bfd_tracer & operator = (const bfd_tracer &);

        private:
            int maxframes;
            int frames;
            void **tbuf;
    };
#endif
#ifdef USE_DWARF
    class dwarf_tracer
    {
        public:
            dwarf_tracer(int _maxframes);

            ~dwarf_tracer();

            const std::string & trace(int skip) const;

            dwarf_tracer(const dwarf_tracer &);

            dwarf_tracer & operator = (const dwarf_tracer &);

        private:
            int maxframes;
            int frames;
            void **tbuf;
    };
#endif

    /**
     * An exception, which can generate a backtrace
     */
    class exception : public std::exception
    {
        public:
            /**
             * Constructs a new backtrace_exception.
             */
            exception() throw();

            virtual ~exception() throw();

            /**
             * Returns a backtrace.
             * @return string with multiple backtrace lines.
             */
            virtual const char* where() const throw();

        private:
#ifdef USE_BFD
            bfd_tracer tracer;
#else
# ifdef USE_DWARF
            dwarf_tracer tracer;
# else
#ifdef _WIN32
#pragma message("Neither libbfd nor libdwarf are available, so no backtracing enabled")
			dummy_tracer tracer;
#else
# warning Neither libbfd nor libdwarf are available, so no backtracing enabled
            dummy_tracer tracer;
#endif
# endif
#endif
    };

    class runtime_error : public exception
    {
        public:
            explicit runtime_error(const std::string& __arg)
                : exception(), msg(__arg) { }
            virtual ~runtime_error() throw() { }
            virtual const char* what() const throw()
            { return msg.c_str(); }

        private:
            std::string msg;
    };
}

#endif
