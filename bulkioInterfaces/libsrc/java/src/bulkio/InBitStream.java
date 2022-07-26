/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of REDHAWK bulkioInterfaces.
 *
 * REDHAWK bulkioInterfaces is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * REDHAWK bulkioInterfaces is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */
/*
 * WARNING: This file is generated from InPort.java.template.
 *          Do not modify directly.
 */
package bulkio;

import org.apache.log4j.Logger;

import org.ossie.component.RHLogger;

import org.ossie.buffer.bitbuffer;

import BULKIO.PortStatistics;
import BULKIO.PortUsageType;
import BULKIO.PrecisionUTCTime;
import BULKIO.StreamSRI;
import bulkio.InStreamBase;
import bulkio.InBitPort;
import bulkio.sri.utils;

import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.Iterator;

import java.nio.ByteBuffer;

public class InBitStream extends InStreamBase {

    protected InBitPort _port;
    protected int _eosState;
    protected int _samplesQueued;
    public int _sampleOffset;
    protected int _queuedPacketIndex;
    protected boolean _enabled;
    protected boolean _newstream;
    protected StreamSRI _sri;
    public ArrayDeque<InBitPort.Packet> _queue;
    
    public InBitStream(BULKIO.StreamSRI sri, InBitPort port) {
        /*
        Create an InBitStream.

        Warning:
            Input streams are created by the input port. This constructor
            should not be called directly.

        See Also:
            InPort.getCurrentStream
            InPort.getStream
        */
        this._port = port;
        this._eosState = InStreamBase.EOS_NONE;
        this._enabled = true;
        this._newstream = true;
        this._samplesQueued = 0;
        this._sampleOffset = 0;
        this._queuedPacketIndex = 0;
        this._sri = sri;
        this._queue = new ArrayDeque<InBitPort.Packet>();
        //self.__blockType = blockType
    }

    public String streamID() {
        if (this._sri != null) {
            return this._sri.streamID;
        }
        return null;
    }

    public BitDataBlock read() {
        /*
        Blocking read of the next packet for this stream.

        The read may fail if:
            * End-of-stream has been reached
            * The input port is stopped
 
        Returns:
            DataBlock if successful.
            None if the read failed.
        */
        return this._readPacket(true);
    }

    public BitDataBlock read(int count) {
        /*
        Blocking read of the next packet for this stream.

        The read may fail if:
            * End-of-stream has been reached
            * The input port is stopped
 
        Returns:
            DataBlock if successful.
            None if the read failed.
        */
        return this.read(count, count, true);
    }

    public BitDataBlock read(int count, int consume) {
        /*
        Blocking read of the next packet for this stream.

        The read may fail if:
            * End-of-stream has been reached
            * The input port is stopped
 
        Returns:
            DataBlock if successful.
            None if the read failed.
        */
        return this.read(count, consume, true);
    }

    public BitDataBlock read(int count, int consume, boolean blocking) {
        BULKIO.StreamSRI sri = _nextSRI(blocking);
        if (sri == null) {
            // No SRI retreived implies no data will be retrieved, either due
            // to end-of-stream or because it would block
            this._reportIfEosReached();
            return null;
        }

        // If the next block of data is complex, double the read and consume size
        // (which the lower-level I/O handles in terms of scalars) so that the
        // returned block has the right number of samples
        if (sri.mode == 1) {
            count *= 2;
            consume *= 2;
        }

        // Queue up packets from the port until we have enough data to satisfy the
        // requested read amount
        this._refreshQueue();

        if (this._samplesQueued == 0) {
            // As above, it's possible that there are no samples due to an end-
            // of-stream
            this._reportIfEosReached();
            return null;
        }

        // Only read as many samples as are available (e.g., if a new SRI is coming
        // or the stream reached the end)
        int samples = Math.min(count, this._samplesQueued);
        int ceiling = Math.max(count, consume);

        if (samples < ceiling) {
            // Non-blocking: return a null block if there's not currently a break in
            // the data, under the assumption that a future read might return the
            // full amount
            if (!blocking && (this._eosState == EOS_NONE)) {
                    return null;
            }
        }

        // Handle a partial read, which could mean that there's not enough data at
        // present (non-blocking), or that the read pointer has reached the end of
        // a segment (new SRI, queue flush, end-of-stream)
        if (samples < count) {
            // Non-blocking: return a null block if there's not currently a break in
            // the data, under the assumption that a future read might return the
            // full amount
            if (!blocking && (this._eosState == EOS_NONE)) {
                    return null;
            }
            // Otherwise, consume all remaining data (when not requested as 0)
            if (consume != 0)
                consume = samples;
        }

        return _readData(samples, consume);
    }

    public BitDataBlock tryread() {
        /*
        Non-blocking version of read().

        The read returns immediately whether data is available or not.

        Returns:
            DataBlock if successful.
            None if no data is available or the read failed.
        */
        return this._readPacket(false);
    }

    public BitDataBlock tryread(int count) {
        /*
        Non-blocking version of read().

        The read returns immediately whether data is available or not.

        Returns:
            DataBlock if successful.
            None if no data is available or the read failed.
        */
        return this.read(count, count, false);
    }

    public BitDataBlock tryread(int count, int consume) {
        /*
        Non-blocking version of read().

        The read returns immediately whether data is available or not.

        Returns:
            DataBlock if successful.
            None if no data is available or the read failed.
        */
        return this.read(count, consume, false);
    }

    public boolean enabled() {
        /*
        bool: Indicates whether this stream can receive data.

        If a stream is enabled, packets received for its stream ID are queued
        in the input port, and the stream may be used for reading. Conversely,
        packets for a disabled stream are discarded, and no reading may be
        performed.
        */
        return this._enabled;
    }

    public void enable() {
        /*
        Enable this stream for reading data.

        The input port will resume queuing packets for this stream.
        */
        // Changing the enabled flag requires holding the port's streamsMutex
        // (that controls access to the stream map) to ensure that the change
        // is atomic with respect to handling end-of-stream packets. Otherwise,
        // there is a race condition between the port's IO thread and the
        // thread that enables the stream--it could be re-enabled and start
        // reading in between the port checking whether to discard the packet
        // and closing the stream. Because it is assumed that the same thread
        // that calls enable is the one doing the reading, it is not necessary
        // to apply mutual exclusion across the entire public stream API, just
        // enable/disable.

        synchronized (this._port.streamsMutex) {
            this._enabled = true;
        }
    }

    public boolean ready()
    {
        return samplesAvailable() > 0;
    }

    public int samplesAvailable()
    {
        this._refreshQueue();
        return this._samplesQueued;
    }
    
    public void disable() {
        /*
        Disable this stream for reading data.
        
        The input port will discard any packets that are currently queued for
        this stream, and all future packets for this stream will be discarded
        upon receipt until an end-of-stream is received.

        Disabling unwanted streams may improve performance and queueing
        behavior by reducing the number of queued packets on a port.
        */

        // Unless end-of-stream has been received by the port (meaning any further
        // packets with this stream ID are for a different instance), purge any
        // packets for this stream from the port's queue
        this._disable();
    }

    public boolean eos() {
        /*
        Checks whether this stream has ended.

        A stream is considered at the end when it has read and consumed all
        data up to the end-of-stream marker. Once end-of-stream has been
        reached, all read operations will fail immediately, as no more data
        will ever be received for this stream.

        The recommended practice is to check @a eos any time a read operation
        fails or returns fewer samples than requested. When the end-of-stream
        is acknowledged, either by checking @a eos or when successive reads
        fail due to an end-of-stream, the stream is removed from the input
        port. If the input port has another stream with the same streamID
        pending, it will become active.

        Returns:
            True if this stream has reached the end.
            False if the end of stream has not been reached.
        */
        return this._checkEos();
    }

    public int skip(int count) {
        // If the next block of data is complex, double the skip size (which the
        // lower-level I/O handles in terms of scalars) so that the right number of
        // samples is skipped
        BULKIO.StreamSRI sri = _nextSRI(true);
        if (sri == null) {
            return 0;
        }

        int item_size = 0;
        if (sri.mode == 1) {
            item_size = 2;  // complex
        } else {
            item_size = 1;  // real
        }
        count *= item_size;

        // Queue up packets from the port until we have enough data to satisfy the
        // requested read amount
        while (this._samplesQueued < count) {
            if (this._fetchPacket(true) == null) {
                break;
            }
        }

        count = Math.min(count, _samplesQueued);
        this._consumeData(count);

        // Convert scalars back to samples
        return count / item_size;
    }

    protected void _addTimestamp(BitDataBlock data, double inputOffset, int outputOffset, BULKIO.PrecisionUTCTime time)
    {
        // Determine the timestamp of this chunk of data; if this is the
        // first chunk, the packet offset (number of samples already read)
        // must be accounted for, so adjust the timestamp based on the SRI.
        // Otherwise, the adjustment is a noop.
        double time_offset = inputOffset * data.xdelta();
        // Check the SRI directly for the complex mode because bit data blocks
        // intentionally do not have a complex() method.
        if (data.sri().mode != 0) {
            // Complex data; each sample is two values
            time_offset /= 2.0;
            outputOffset /= 2;
        }

        // If there is a time offset, apply the adjustment and mark the timestamp
        // so that the caller knows it was calculated rather than received
        boolean synthetic = false;
        if (time_offset > 0.0) {
            time = bulkio.time.utils.add(time, time_offset);
            synthetic = true;
        }

        data.addTimestamp(time, outputOffset, synthetic);
    }

    protected void _setBlockFlags(BitDataBlock block, InBitPort.Packet packet)
    {
        // Allocate empty data block and propagate the SRI change and input
        // queue flush flags
        int flags=0;
        if (packet.sriChanged) {
            flags = bulkio.sri.utils.compareFields(this._sri, packet.SRI);
            block.setSriChangeFlags(flags);
        }
        if (_newstream) {
            _newstream=false;
            flags = flags | bulkio.sri.utils.STREAMID | bulkio.sri.utils.XDELTA | bulkio.sri.utils.YDELTA | bulkio.sri.utils.KEYWORDS | bulkio.sri.utils.MODE;
            block.setSriChangeFlags(flags);
        }
        if (packet.inputQueueFlushed) {
            block.setInputQueueFlushed(true);
        }
    }

    private static Boolean isBitSet(byte b, int bit)
    {
        return (b & (1 << bit)) != 0;
    }

    public bitbuffer byteToBitBuffer(byte[] bytearray, int nbits){
        bitbuffer returnValue = new bitbuffer(bytearray.length*8);
        ByteBuffer  byteBuffer = ByteBuffer.wrap(bytearray);
        //Hexadecimal values used are Big-Endian, because Java is Big-Endian
        for (int i = 0; i < bytearray.length; i++) {
            byte thebyte = byteBuffer.get(i);
            for (int j = 0; j <8 ; j++) {
                returnValue.set(i*8+j,isBitSet(thebyte,j));
            }
        }
        returnValue.length = nbits;
        return returnValue;
    }

    // count and consumer are in terms of bits
    protected BitDataBlock _readData(int count, int consume)
    {

        int max_val = Math.max(count, consume);
        while (max_val > this._samplesQueued) {
            try {
                Thread.sleep(1);
            } catch (InterruptedException e) {
                return null;
            }
            this._refreshQueue();
        }
        // Acknowledge pending SRI change
        InBitPort.Packet front = this._queue.peekFirst();

        // Allocate empty data block and propagate the SRI change and input queue
        // flush flags
        bitbuffer data_payload = null;
        BitDataBlock data = new BitDataBlock(front.SRI, data_payload, 0, false);
        this._setBlockFlags(data, front);
        if (front.sriChanged) {
            // Update the stream metadata
            data.setSri(front.SRI);
        }

        int last_offset = this._sampleOffset + count;
        bitbuffer front_dataBuffer = byteToBitBuffer(front.dataBuffer.data, front.dataBuffer.bits);
        if (last_offset <= front_dataBuffer.length) {
            // The requsted sample count can be satisfied from the first packet
            _addTimestamp(data, (double)_sampleOffset, 0, front.T);
            data_payload = new bitbuffer(count);
            for (int i=0; i<data_payload.length; i++) {
                if (front_dataBuffer.get(i+this._sampleOffset)) {
                    data_payload.set(i);
                }
            }
            data.setBuffer(data_payload);
        } else {
            // We have to span multiple packets to get the data
            data_payload = new bitbuffer(count);
            int data_offset = 0;

            // Assemble data spanning several input packets into the output buffer
            int packet_bit_offset = _sampleOffset;
            int bit_count = 0;
            Iterator<InBitPort.Packet> itr = _queue.iterator();
            while (count > 0) {
                while (!itr.hasNext()) {
                    try {
                        Thread.sleep(1);
                    } catch (InterruptedException e) {
                        return null;
                    }
                }
                InBitPort.Packet packet = itr.next();
                // Add the timestamp for this pass
                _addTimestamp(data, (double)packet_bit_offset, bit_count, packet.T);
                bitbuffer input_data = byteToBitBuffer(packet.dataBuffer.data, packet.dataBuffer.bits);
                //bitbuffer input_data = packet.dataBuffer;
                int available_data = input_data.length - packet_bit_offset;
                for (int i=0; i<available_data; i++) {
                    if (input_data.get(i+packet_bit_offset)) {
                        data_payload.set(bit_count);
                    }
                    bit_count++;
                    count--;
                    if (count == 0) {
                        break;
                    }
                }
                packet_bit_offset = 0;
            }
            data.setBuffer(data_payload);
        }
        // Clear flags from packet, since they've been reported
        front.sriChanged = false;
        front.inputQueueFlushed = false;

        // Advance the read pointers
        _consumeData(consume);

        return data;
    }

    protected void _disable() {
        this._queue.clear();
        this._sampleOffset = 0;
        this._samplesQueued = 0;
        this._enabled = false;
        if (this._eosState == EOS_NONE) {
            this._port._discardPacketsForStream(this._sri.streamID);
        }
    }

    protected boolean _checkEos() {
        // Internal method to check for end-of-stream. Subclasses should extend
        //  or override this method.

        this._reportIfEosReached();
        //  At this point, if end-of-stream has been reached, the state is
        //  reported (it gets set above), so the checking for the latter is
        //  sufficient
        boolean retval = (this._eosState == EOS_REPORTED);
        return retval;
    }

    protected boolean _hasBufferedData() {
        this._refreshQueue();
        if (!_queue.isEmpty()) {
                // Return true if either there are queued or pending packets
            return true;
        }
        // For the base class, there is no data to report; however, to nudge
        // the check end-of-stream, return true if it has been reached but not
        // reported
        boolean retval = (this._eosState == EOS_REACHED);
        return retval;
    }

    protected boolean _queuePacket(InBitPort.Packet packet) {
        if ((packet.EOS) && (packet.dataBuffer.data.length == 0)) {
            // Handle end-of-stream packet with no data (assuming that
            // timestamps, SRI changes, and queue flushes are irrelevant at this
            // point)
            if (this._queue.isEmpty()){ 
                // No queued packets, read pointer has reached end-of-stream
                this._eosState = EOS_REACHED;
            } else {
                // Assign the end-of-stream flag to the last packet in the queue
                // so that it is handled on read
                this._queue.peekLast().EOS = true;
            }
            // Let the caller know that no more sample data is forthcoming
            return false;
        } else {
            // Add the packet to the queue
            this._samplesQueued += packet.dataBuffer.bits;
            this._queue.add(packet);
            return true;
        }
    }

    protected void _close() {
        // NB: This method is always called by the port with streamsMutex held
        // Consider end-of-stream reported, since the stream has already been
        // removed from the port; otherwise, there's nothing to do
        this._eosState = EOS_REPORTED;
    }

    //this is where the eos needs to be processed wrt read eos vs retrieve data
    protected BitDataBlock _readPacket(boolean blocking) {
        InBitPort.Packet packet = this._fetchNextPacket(blocking);
        if (this._eosState == InStreamBase.EOS_RECEIVED) {
            this._eosState = InStreamBase.EOS_REACHED;
        }
        if (packet == null) {
            this._reportIfEosReached();
            return null;
        }
        if (packet.EOS && (packet.dataBuffer.data.length == 0)) {
            this._reportIfEosReached();
            return null;
        }

        // Turn packet into a data block
        int sri_flags = this._getSriChangeFlags(packet);
        bitbuffer _data = byteToBitBuffer(packet.dataBuffer.data, packet.dataBuffer.bits);
        BitDataBlock block = new BitDataBlock(packet.SRI, _data, sri_flags, packet.inputQueueFlushed);
        // Only add a timestamp if one was given (XML does not include one in
        //  the CORBA interface, but passes a None to adapt to the common port
        //  implementation)
        if (packet.T != null) {
            block.addTimestamp(packet.T, 0, false);
        }
        if (this._sampleOffset != 0) {
            _addTimestamp(block, (double)_sampleOffset, 0, packet.T);
            bitbuffer data_payload = new bitbuffer(_data.length-this._sampleOffset);
            for (int i=0; i<data_payload.length; i++) {
                if (_data.get(i+this._sampleOffset)) {
                    data_payload.set(i);
                }
            }
            block.setBuffer(data_payload);
        }
        this._samplesQueued = this._samplesQueued - (_data.length-this._sampleOffset);
        this._sampleOffset = 0;

        // Update local SRI from packet
        this._sri = packet.SRI;
        return block;
    }

    protected boolean _canBridge(InBitPort.Packet packet) {
        boolean retval = (packet.sriChanged || packet.inputQueueFlushed);
        return (!retval);
    }

    protected InBitPort.Packet _fetchPacket(boolean blocking) {
        return this._fetchNextPacket(blocking);
    }

    protected void _refreshQueue() {
        InBitPort.Packet packet = null;
        if ((this._eosState != InStreamBase.EOS_REACHED) && (this._eosState != InStreamBase.EOS_RECEIVED)) {
            do {
                packet = this._port.fetchPacket(this._sri.streamID);
                if (packet != null) {
                    // TODO: Data conversion (no-op for all types except dataChar/dataByte)
                    //packet.dataBuffer = this._port._reformat(packet.dataBuffer);
                    this._queuePacket(packet);
                    if (packet.EOS) {
                        this._eosState = InStreamBase.EOS_RECEIVED;
                        break;
                    }
                }
            } while (packet != null);
        }
        if (this._queue.size() <= 1) {
            if (this._eosState == InStreamBase.EOS_RECEIVED) {
                this._eosState = InStreamBase.EOS_REACHED;
            }
        }
    }

    protected InBitPort.Packet _fetchNextPacket(boolean blocking) {
            // Don't fetch a packet from the port if stream is disabled
        if (!this._enabled) {
            return null;
        }

        // Any future packets with this stream ID belong to another InputStream
        if ((this._eosState != InStreamBase.EOS_NONE) && (this._eosState != InStreamBase.EOS_RECEIVED) && (this._eosState != InStreamBase.EOS_REACHED)) {
            return null;
        }

        long timeout = 0;
        if (blocking) {
            timeout = bulkio.Const.BLOCKING;
        } else {
            timeout = bulkio.Const.NON_BLOCKING;
        }

        this._refreshQueue();

        return this._queue.poll();
    }

    protected void _reportIfEosReached() {
        if (this._eosState == InStreamBase.EOS_REACHED) {
            /* This is the first time end-of-stream has been checked since it
                was reached; remove the stream from the port now, since the
                caller knows that the stream ended */
            // TODO
            this._port._removeStream(this._sri.streamID);
            this._eosState = InStreamBase.EOS_REPORTED;
        }
    }

    protected int _getSriChangeFlags(InBitPort.Packet packet) {
        if (this._newstream) {
            this._newstream = false;
            return -1;
        } else if (packet.sriChanged) {
            return bulkio.sri.utils.compareFields(this._sri, packet.SRI);
        } else {
            return bulkio.sri.utils.NONE;
        }
    }

    protected BULKIO.StreamSRI _nextSRI(boolean blocking) {
        this._refreshQueue();
        if (this._queue.isEmpty()) {
            if (_fetchPacket(blocking) == null) {
                return null;
            }
        }

        return this._queue.peekFirst().SRI;
    }

    //consume bits
    protected void _consumeData(int count) {
        this._refreshQueue();
        while (count > 0) {
            //bitbuffer data = this._queue.peekFirst().dataBuffer;
            bitbuffer data = byteToBitBuffer(this._queue.peekFirst().dataBuffer.data, this._queue.peekFirst().dataBuffer.bits);

            int available = data.length - this._sampleOffset;
            int pass = Math.min(available, count);

            this._sampleOffset += pass;
            this._samplesQueued -= pass;
            count -= pass;

            if (_sampleOffset >= data.length) {
                // Read pointer has passed the end of the packet data
                _consumePacket();
            }
        }
    }

    protected void _consumePacket() {
        // Acknowledge any end-of-stream flag and delete the packet (the queue will
        // automatically delete it when it's removed)
        if (_queue.peekFirst().EOS) {
            this._eosState = InStreamBase.EOS_REACHED;
        }
        _queue.poll();
        this._sampleOffset = 0;
    }
};
