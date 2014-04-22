/* $Id: sha1.h 85 2012-03-22 12:17:32Z felfert $
 *
 *  sha1.h
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved.
 *
 *****************************************************************************
 *  $Id: sha1.h 85 2012-03-22 12:17:32Z felfert $
 *****************************************************************************
 *
 *  Description:
 *      This class implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      Many of the variable names in this class, especially the single
 *      character names, were used because those were the names used
 *      in the publication.
 *
 *      Please read the file sha1.cpp for more information.
 *
 */

#ifndef _SHA1_H_
#define _SHA1_H_

/**
 *      This class implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      The Secure Hashing Standard, which uses the Secure Hashing
 *      Algorithm (SHA), produces a 160-bit message digest for a
 *      given data stream.  In theory, it is highly improbable that
 *      two messages will produce the same message digest.  Therefore,
 *      this algorithm can serve as a means of providing a "fingerprint"
 *      for a message.
 *
 *  <h2>Portability Issues</h2>
 *      SHA-1 is defined in terms of 32-bit "words".  This code was
 *      written with the expectation that the processor has at least
 *      a 32-bit machine word size.  If the machine word size is larger,
 *      the code should still function properly.  One caveat to that
 *      is that the input functions taking characters and character arrays
 *      assume that only 8 bits of information are stored in each character.
 *
 *  <h2>Caveats</h2>
 *      SHA-1 is designed to work with messages less than 2^64 bits long.
 *      Although SHA-1 allows a message digest to be generated for
 *      messages of any number of bits less than 2^64, this implementation
 *      only works with messages with a length that is a multiple of 8
 *      bits.
 */
class SHA1
{

    public:

        SHA1();
        virtual ~SHA1();

        /**
         * Initializes the class member variables
         * in preparation for computing a new message digest.
         */
        void Reset();

        /**
         * Retrieves the 160-bit message digest.
         * @param message_digest_array A pointer to an array, holding the digest.
         * This array <b>MUST</b> hold 5 unsigned ints.
         * @return true on success.
         */
        bool Result(unsigned *message_digest_array);

        /**
         * Append data to the the message.
         * @param message_array Array of octets.
         * @param length The amount of octets to add.
         */
        void Input( const unsigned char *message_array,
                    unsigned            length);
        /**
         * Append data to the the message.
         * @param message_array Array of octets.
         * @param length The amount of octets to add.
         */
        void Input( const char  *message_array,
                    unsigned    length);
        /**
         * Append data to the the message.
         * @param message_element A single octet.
         */
        void Input(unsigned char message_element);
        /**
         * Append data to the the message.
         * @param message_element A single octet.
         */
        void Input(char message_element);
        /**
         * Convenience operator for appending string data to the the message.
         * @param message_array A string.
         * @return A reference to this instance.
         */
        SHA1& operator<<(const char *message_array);
        /**
         * Convenience operator for appending string data to the the message.
         * @param message_array A string.
         * @return A reference to this instance.
         */
        SHA1& operator<<(const unsigned char *message_array);
        /**
         * Convenience operator for appending character data to the the message.
         * @param message_element A character.
         * @return A reference to this instance.
         */
        SHA1& operator<<(const char message_element);
        /**
         * Convenience operator for appending octet data to the the message.
         * @param message_element An octet.
         * @return A reference to this instance.
         */
        SHA1& operator<<(const unsigned char message_element);

    private:

        /*
         *  Process the next 512 bits of the message
         */
        void ProcessMessageBlock();

        /*
         *  Pads the current message block to 512 bits
         */
        void PadMessage();

        /*
         *  Performs a circular left shift operation
         */
        inline unsigned CircularShift(int bits, unsigned word);

        unsigned H[5];                      // Message digest buffers

        unsigned Length_Low;                // Message length in bits
        unsigned Length_High;               // Message length in bits

        unsigned char Message_Block[64];    // 512-bit message blocks
        int Message_Block_Index;            // Index into message block array

        bool Computed;                      // Is the digest computed?
        bool Corrupted;                     // Is the message digest corruped?
    
};

#endif
