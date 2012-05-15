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
#ifndef BTEXCEPTION_H
#define BTEXCEPTION_H

#include <exception>
#include <string>

/**
 * This namespace encapsulates our custom
 * tracing exception and their derived classes.
 */
namespace tracing {

    /**
     * An empty backtrace generator, which gets used,
     * when neither BFD nor Dwarf are available.
     */
    class dummy_tracer
    {
        public:
            /// Constructor
            dummy_tracer(int) { }

            /**
             * Dummy trace generator.
             * @return A string constant "Tracing disabled".
             */
            std::string trace(int) const
            {
                return "Tracing disabled";
            }
    };

#ifdef USE_BFD

    /**
     * Generic backtrace genrator using BFD.
     * This class can create a human readable
     * backtrace.
     */
    class bfd_tracer
    {
        public:

            /**
             * Constructs a new instance, representing the current stack.
             * @param _maxframes The maximum number of stack frames to capture.
             */
            bfd_tracer(int _maxframes);

            /**
             * Releases all internal resources.
             */
            ~bfd_tracer();

            /**
             * Produces a multi-line human readable stack trace.
             * If the current executable contains debug symbol
             * information the function names and source line numbers
             * are used, otherwise, the stack trace contains only
             * hexadecimal addresses.
             * @param skip Number of frames to skip.
             * @return A multi line string, containing the backtrace.
             */
            const std::string & trace(int skip) const;

            /** Copy constructor */
            bfd_tracer(const bfd_tracer &);

        private:
            /** Size of our frame array */
            int maxframes;
            /** Number of frames actually used by the current backtrace */
            int frames;
            /** The frame array from backtrace() */
            void **tbuf;
            /** Disabled assignment operator */
            bfd_tracer & operator = (const bfd_tracer &);

    };
#endif
#ifdef USE_DWARF
    /**
     * Generic backtrace genrator using Dwarf.
     * This class can create a human readable
     * backtrace.
     */
    class dwarf_tracer
    {
        public:
            /**
             * Constructs a new instance, representing the current stack.
             * @param _maxframes The maximum number of stack frames to capture.
             */
            dwarf_tracer(int _maxframes);

            /**
             * Releases all internal resources.
             */
            ~dwarf_tracer();

            /**
             * Produces a multi-line human readable stack trace.
             * If the current executable contains debug symbol
             * information the function names and source line numbers
             * are used, otherwise, the stack trace contains only
             * hexadecimal addresses.
             * @param skip Number of frames to skip.
             * @return A multi line string, containing the backtrace.
             */
            const std::string & trace(int skip) const;

            /** Copy constructor */
            dwarf_tracer(const dwarf_tracer &);

            /// Assignement operator
            dwarf_tracer & operator = (const dwarf_tracer &);

        private:
            int maxframes;
            int frames;
            void **tbuf;
    };
#endif

    /**
     * An exception, which can generate a backtrace.
     * This class uses either bfd_tracer or dwarf_tracer
     * in order to produce a backtrace, if an exception is thrown.
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
# warning Neither libbfd nor libdwarf are available, so no backtracing enabled
            dummy_tracer tracer;
# endif
#endif
    };

    /** Runtime errors represent problems outside the scope of a program;
     *  they cannot be easily predicted and can generally only be caught as
     *  the program executes.
     *  @brief One of two subclasses of exception.
     */
    class runtime_error : public exception
    {
        public:
            /**
             * Constructor
             * @param __arg The message to be held by this instance.
             */
            explicit runtime_error(const std::string& __arg)
                : exception(), msg(__arg) { }
            /// Destructor
            virtual ~runtime_error() throw() { }
            /**
             * Retrieves the message, provided at construction time.
             * @return The textual message, provided at construction time.
             */
            virtual const char* what() const throw()
            { return msg.c_str(); }

        private:
            std::string msg;
    };

    /** Logic errors represent problems in the internal logic of a program;
     *  in theory, these are preventable, and even detectable before the
     *  program runs (e.g., violations of class invariants).
     *  @brief One of two subclasses of exception.
     */
    class logic_error : public exception
    {
        std::string _M_msg;

        public:
        /** Constructor.
         * @param __arg The message to be held by this instance.
         */
        explicit logic_error(const std::string&  __arg) : _M_msg(__arg) { }
        /// Destructor
        virtual ~logic_error() throw() { }
        /**
         * Retrieves the message, provided at construction time.
         * @return The textual message, provided at construction time.
         */
        virtual const char* what() const throw() { return _M_msg.c_str(); }
    };

    /** Thrown by the library, or by you, to report domain errors (domain in
     *  the mathmatical sense).
     */
    class domain_error : public logic_error 
    {
        public:
            /** Constructor.
             * @param __arg The message to be held by this instance.
             */
            explicit domain_error(const std::string&  __arg);
    };

    /** Thrown to report invalid arguments to functions.  */
    class invalid_argument : public logic_error 
    {
        public:
            /** Constructor.
             * @param __arg The message to be held by this instance.
             */
            explicit invalid_argument(const std::string&  __arg) : logic_error(__arg) { }
    };

    /** Thrown when an object is constructed that would exceed its maximum
     *  permitted size (e.g., a basic_string instance).  */
    class length_error : public logic_error 
    {
        public:
            /** Constructor.
             * @param __arg The message to be held by this instance.
             */
            explicit length_error(const std::string&  __arg);
    };

    /** This represents an argument whose value is not within the expected
     *  range (e.g., boundary checks in basic_string).  */
    class out_of_range : public logic_error 
    {
        public:
            /** Constructor.
             * @param __arg The message to be held by this instance.
             */
            explicit out_of_range(const std::string&  __arg);
    };


    /** Thrown to indicate range errors in internal computations.  */
    class range_error : public runtime_error 
    {
        public:
            /** Constructor.
             * @param __arg The message to be held by this instance.
             */
            explicit range_error(const std::string&  __arg);
    };

    /** Thrown to indicate arithmetic overflow.  */
    class overflow_error : public runtime_error 
    {
        public:
            /** Constructor.
             * @param __arg The message to be held by this instance.
             */
            explicit overflow_error(const std::string&  __arg);
    };

    /** Thrown to indicate arithmetic underflow.  */
    class underflow_error : public runtime_error 
    {
        public:
            /** Constructor.
             * @param __arg The message to be held by this instance.
             */
            explicit underflow_error(const std::string&  __arg);
    };

}

#endif
