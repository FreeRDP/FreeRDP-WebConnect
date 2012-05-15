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
#ifndef _LOGGING_H_
#define _LOGGING_H_

# include <ostream>
# include <string>
# include <bitset>

namespace wsgate {

    /**
     * A logging class, mapping syslog(3) functionality to C++ on Unix
     * systems. On Windows systems, the logging is performed via the
     * application event log (all log levels, except LOG_DEBUG) and via
     * OutputDebugString (for log level LOG_DEBUG).
     * For every log level, a corresponding ostream exists.
     */
    class logger {
        public:

            /// The predefined facilities
            typedef enum {
                AUTH,
                AUTHPRIV,
                CRON,
                DAEMON,
                FTP,
                KERN,
                LOCAL0,
                LOCAL1,
                LOCAL2,
                LOCAL3,
                LOCAL4,
                LOCAL5,
                LOCAL6,
                LOCAL7,
                LPR,
                MAIL,
                NEWS,
                SYSLOG,
                USER,
                UUCP
            } Facility;

            /**
             * Initializes logging.
             * @param ident The program identifier. On Windows systems,
             * this parameter defines the name of the EventSource in the application log.<br />
             * <b>Additional note for Windows:</b><br />
             * In order to create proper entries in the application event log, a registry
             * key with the supplied name is created below
             * HKLM\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\
             * this key is <b>not</b> automatically deleted, since it's content points to
             * the executable (or dll) which is using this class. Therefore, in your
             * uninstaller, please remove that key.
             * @see openlog(3).
             * @param facility The log facility to use. Ignored on Windows. @see openlog(3).
             * @param mask A string, specifying which log levels are enabled.
             *  This string can be specified in multiple formats:
             *  <ul>
             *  <li>A std::bitmask initializer of length 8. The leftmost bit is accociated with the LOG_DEBUG
             *  level, the next bit with LOG_INFO and so on.
             *  <li>A string of log level names (all uppercase). Log level names can be DEBUG, INFO, NOTICE, WARNING, ERR, CRIT, ALERT and EMERG.
             *  </ul>
             */
            logger(const std::string & ident, const Facility facility = DAEMON, const std::string & mask = "11111111");

            /**
             * Copy constructor
             */
            logger(const logger &other);

            ~logger();

            /**
             * Assignment operator
             */
            logger & operator=(const logger &other);

            /**
             * Releases the internal log buffers.
             */
            static void release();

            /**
             * Enables logging on all existing log channels.
             */
            static void enable();

            /**
             * Disables logging on all existing log channels.
             */
            static void disable();

            /**
             * Modifies the set of enabled log levels.
             * @param mask A std::bitset of length 8. The leftmost bit is accociated with the LOG_DEBUG
             *  level, the next bit with LOG_INFO and so on.
             */
            void setmask(std::bitset<8>mask);

            /**
             * Modifies the set of enabled log levels.
             * @param names A string of log level names (all uppercase). Log level names can be DEBUG, INFO, NOTICE, WARNING, ERR, CRIT, ALERT and EMERG.
             */
            void setmaskByName(const std::string & names);

            /**
             * Modifies the log facility.
             * This method has no effect on Windows.
             * @param facility The log facility to use @see openlog(3)
             */
            void setfacility(Facility facility);

            /**
             * Modifies the log facility.
             * This method has no effect on Windows.
             * @param facility A string containg the facility name.
             */
            void setfacilityByName(const std::string & facility);

            /**
             * Stream for logging with log level LOG_DEBUG.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             * On Windows, output is done via
             * <a href="http://tiny.cc/cvrvcw">OutputDebugString</a> and can be examined by tools
             * like <a href="http://tiny.cc/54rvcw">DebugView</a>.
             */
            static std::ostream debug;

            /**
             * Stream for logging with log level LOG_INFO.
             * On Windows, the message is tagged with an
             * Info-Icon in the application event log.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream info;

            /**
             * Stream for logging with log level LOG_NOTICE.
             * On Windows, the message is tagged with an
             * Info-Icon in the application event log.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream notice;

            /**
             * Stream for logging with log level LOG_WARNING.
             * On Windows, the message is tagged with a
             * Warning-Icon in the application event log.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream warn;

            /**
             * Stream for logging with log level LOG_ERR.
             * On Windows, the message is tagged with an
             * Error-Icon in the application event log.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream err;

            /**
             * Stream for logging with log level LOG_CRIT.
             * On Windows, the message is tagged with an
             * Error-Icon in the application event log.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream crit;

            /**
             * Stream for logging with log level LOG_ALERT.
             * On Windows, the message is tagged with an
             * Error-Icon in the application event log.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream alert;

            /**
             * Stream for logging with log level LOG_EMERG.
             * Writing to this stream before invokation
             * On Windows, the message is tagged with an
             * Error-Icon in the application event log.
             * of the constructor does nothing.
             */
            static std::ostream emerg;

        private:
            /**
             * Converts log level names to a bitset,suitable for invoking setmask(std::bitset<8>mask).
             * @param names A string of log level names (all uppercase). Log level names can be DEBUG, INFO, NOTICE, WARNING, ERR, CRIT, ALERT and EMERG.
             * @return The converted bitset.
             */
            std::bitset<8> namesToBitset(std::string names);

            /**
             * Performs the actual initialization.
             * @param facility The log facility to use @see openlog(3)
             * @param mask A std::bitset of length 8. The leftmost bit is accociated with the LOG_DEBUG
             *  level, the next bit with LOG_INFO and so on.
             */
            void init(Facility facility, std::bitset<8>mask);

            char *ident;

            std::bitset<8> mask;
    };

}

#endif
