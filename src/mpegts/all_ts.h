//
// Created by root on 17-5-3.
//

#ifndef PRACTICE_SERVER_ALL_TS_H
#define PRACTICE_SERVER_ALL_TS_H

#include <p_core_error.h>

#ifndef __STDC_FORMAT_MACROS
    #define __STDC_FORMAT_MACROS
#endif

enum TSPidTable
{
    // Program Association Table(see Table 2-25).
    TSPidTablePAT    = 0x00,
    // Conditional Access Table (see Table 2-27).
    TSPidTableCAT    = 0x01,
    // Transport Stream Description Table
    TSPidTableTSDT    = 0x02,
    // null packets (see Table 2-3)
    TSPidTableNULL    = 0x01FFF,
};

/*adaptation_field_control*/
/**
* Table 2-5 – Adaptation field control values, page 38.
*/
enum TSAdaptionType
{
    // Reserved for future use by ISO/IEC
    TSAdaptionTypeReserved        = 0x00,
    // No adaptation_field, payload only
    TSAdaptionTypePayloadOnly     = 0x01,
    // Adaptation_field only, no payload
    TSAdaptionTypeAdaptionOnly     = 0x02,
    // Adaptation_field followed by payload
    TSAdaptionTypeBoth            = 0x03,
};
#endif

// Table 2-29 – Stream type assignments. page 66.
enum TSStreamType
{
    // ITU-T | ISO/IEC Reserved
    TSStreamTypeReserved                = 0x00,
    /*defined by ffmpeg*/
    TSStreamTypeVideoMpeg1              = 0x01,
    TSStreamTypeVideoMpeg2              = 0x02,
    TSStreamTypeAudioMpeg1              = 0x03,
    TSStreamTypeAudioMpeg2              = 0x04,
    TSStreamTypePrivateSection          = 0x05,
    TSStreamTypePrivateData             = 0x06,
    TSStreamTypeAudioAAC                = 0x0f,
    TSStreamTypeVideoMpeg4              = 0x10,
    TSStreamTypeVideoH264               = 0x1b,
    TSStreamTypeAudioAC3                = 0x81,
    TSStreamTypeAudioDTS                = 0x8a,
};

/**
* the actually parsed type.
*/
enum TSPidType
{
    TSPidTypeReserved = 0, // TSPidTypeReserved, nothing parsed, used reserved.

    TSPidTypePAT, // Program associtate table
    TSPidTypePMT, // Program map table.

    TSPidTypeVideo, // only for H264 video
    TSPidTypeAudio, // only for AAC audio
};

/**
* Table 2-18 – Stream_id assignments. page 52.
*/
enum TSPESStreamId
{
    PES_program_stream_map          = 0xbc, // 0b10111100
    PES_private_stream_1            = 0xbd, // 0b10111101
    PES_padding_stream              = 0xbe, // 0b10111110
    PES_private_stream_2            = 0xbf, // 0b10111111

    // 110x xxxx
    // ISO/IEC 13818-3 or ISO/IEC 11172-3 or ISO/IEC 13818-7 or ISO/IEC
    // 14496-3 audio stream number x xxxx
    // (stream_id>>5)&0x07 == PES_audio_prefix
    PES_audio_prefix                = 0x06, // 0b110

    // 1110 xxxx
    // ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 or ISO/IEC
    // 14496-2 video stream number xxxx
    // (stream_id>>4)&0x0f == PES_audio_prefix
    PES_video_prefix                = 0x0e, // 0b1110

    PES_ECM_stream                  = 0xf0, // 0b11110000
    PES_EMM_stream                  = 0xf1, // 0b11110001
    PES_DSMCC_stream                = 0xf2, // 0b11110010
    PES_13522_stream                = 0xf3, // 0b11110011
    PES_H_222_1_type_A              = 0xf4, // 0b11110100
    PES_H_222_1_type_B              = 0xf5, // 0b11110101
    PES_H_222_1_type_C              = 0xf6, // 0b11110110
    PES_H_222_1_type_D              = 0xf7, // 0b11110111
    PES_H_222_1_type_E              = 0xf8, // 0b11111000
    PES_ancillary_stream            = 0xf9, // 0b11111001
    PES_SL_packetized_stream        = 0xfa, // 0b11111010
    PES_FlexMux_stream              = 0xfb, // 0b11111011
    // reserved data stream
    // 1111 1100 … 1111 1110
    PES_program_stream_directory    = 0xff, // 0b11111111
};

/**
* Table 35 – Sampling frequency dependent on
* sampling_frequency_index. in page 46.
*/
enum TSAacSampleFrequency
{
    TSAacSampleFrequency96000         = 0x00,
    TSAacSampleFrequency88200         = 0x01,
    TSAacSampleFrequency64000         = 0x02,
    TSAacSampleFrequency48000         = 0x03,
    TSAacSampleFrequency44100         = 0x04,
    TSAacSampleFrequency32000         = 0x05,
    TSAacSampleFrequency24000         = 0x06,
    TSAacSampleFrequency22050         = 0x07,
    TSAacSampleFrequency16000         = 0x08,
    TSAacSampleFrequency12000         = 0x09,
    TSAacSampleFrequency11025         = 0x0a,
    TSAacSampleFrequency8000          = 0x0b,
    TSAacSampleFrequencyReserved0     = 0x0c,
    TSAacSampleFrequencyReserved1     = 0x0d,
    TSAacSampleFrequencyReserved2     = 0x0e,
    TSAacSampleFrequencyReserved3     = 0x0f,
};



#define TS_PACKET_SIZE          188
/**
* logic ts pid.
*/
struct TSPid
{
    TSPidType type;
    // page 66.
    TSStreamType stream_type;
    // page 36
    TSPidTable pid;
    // page 36
    u_int8_t continuity_counter;
};


// TSPacket declares.
class TSPacket
{
public:
    TSHeader* header;
    TSAdaptionField* adaption_field;
    TSPayload* payload;

    TSPacket();
    virtual ~TSPacket();
    int demux(TSContext* ctx, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg);
    int finish();
};

class all_ts {

};

/**
* 2.4.3.2 Transport Stream packet layer. page 36.
*/
class TSHeader
{
public:
    // 1B
    int8_t sync_byte; //8bits
    // 2B
    int8_t transport_error_indicator; //1bit
    int8_t payload_unit_start_indicator; //1bit
    int8_t transport_priority; //1bit
    TSPidTable pid; //13bits
    // 1B
    int8_t transport_scrambling_control; //2bits
    TSAdaptionType adaption_field_control; //2bits
    u_int8_t continuity_counter; //4bits

    TSHeader();
    virtual ~TSHeader();
    int get_size();
    int demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg);
};

// variant ts packet adation field. page 40.
class TSAdaptionField
{
public:
    // 1B
    u_int8_t adaption_field_length; //8bits
    // 1B
    int8_t discontinuity_indicator; //1bit
    int8_t random_access_indicator; //1bit
    int8_t elementary_stream_priority_indicator; //1bit
    int8_t PCR_flag; //1bit
    int8_t OPCR_flag; //1bit
    int8_t splicing_point_flag; //1bit
    int8_t transport_private_data_flag; //1bit
    int8_t adaptation_field_extension_flag; //1bit

    // if PCR_flag, 6B
    int64_t program_clock_reference_base; //33bits
    //6bits reserved.
    int16_t program_clock_reference_extension; //9bits

    // if OPCR_flag, 6B
    int64_t original_program_clock_reference_base; //33bits
    //6bits reserved.
    int16_t original_program_clock_reference_extension; //9bits

    // if splicing_point_flag, 1B
    int8_t splice_countdown; //8bits

    // if transport_private_data_flag, 1+p[0] B
    u_int8_t transport_private_data_length; //8bits
    char* transport_private_data; //[transport_private_data_length]bytes

    // if adaptation_field_extension_flag, 2+x bytes
    u_int8_t adaptation_field_extension_length; //8bits
    int8_t ltw_flag; //1bit
    int8_t piecewise_rate_flag; //1bit
    int8_t seamless_splice_flag; //1bit
    //5bits reserved
    // if ltw_flag, 2B
    int8_t ltw_valid_flag; //1bit
    int16_t ltw_offset; //15bits
    // if piecewise_rate_flag, 3B
    //2bits reserved
    int32_t piecewise_rate; //22bits
    // if seamless_splice_flag, 5B
    int8_t splice_type; //4bits
    int8_t DTS_next_AU0; //3bits
    int8_t marker_bit0; //1bit
    int16_t DTS_next_AU1; //15bits
    int8_t marker_bit1; //1bit
    int16_t DTS_next_AU2; //15bits
    int8_t marker_bit2; //1bit
    // left bytes.
    char* af_ext_reserved;

    // left bytes.
    char* af_reserved;

    // user defined total adaption field size.
    int __field_size;
    // logic pcr/original_pcr
    int64_t pcr;
    int64_t original_pcr;

    TSAdaptionField();
    virtual ~TSAdaptionField();
    int get_size();
    int demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg);
};

// variant ts packet payload.
// PES packet or PSI table.
// TSPayloadPAT: page 61.
class TSPayload
{
public:
    /**
    * the size of payload(payload plush the 1byte pointer_field).
    */
    int size;
    int pointer_field_size;

    TSPidType type;

    /**
    * 2.4.4.2 Semantics definition of fields in pointer syntax
    */
    u_int8_t pointer_field;

    TSPayloadReserved* reserved;
    TSPayloadPAT* pat;
    TSPayloadPMT* pmt;
    TSPayloadPES* pes;

    /**
    * 2.4.3.6 PES packet. page 49.
    */

    TSPayload();
    virtual ~TSPayload();;
    void read_pointer_field(TSPacket* pkt, u_int8_t*& p);
    int demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg);
};


/**
* 2.4.4.3 Program association Table. page 61.
*/
class TSPayloadPAT
{
public:
    // 1B
    u_int8_t table_id; //8bits

    // 2B
    int8_t section_syntax_indicator; //1bit
    int8_t const0_value; //1bit
    // 2bits reserved.
    u_int16_t section_length; //12bits

    // 2B
    u_int16_t transport_stream_id; //16bits

    // 1B
    // 2bits reerverd.
    int8_t version_number; //5bits
    int8_t current_next_indicator; //1bit

    // 1B
    u_int8_t section_number; //8bits

    // 1B
    u_int8_t last_section_number; //8bits

    // multiple 4B program data.
    // program_number 16bits
    // reserved 2bits
    // 13bits data: 0x1FFF
    // if program_number program_map_PID 13bits
    // else network_PID 13bytes.
    int program_size;
    int32_t* programs; //32bits

    // 4B
    int32_t CRC_32; //32bits

    TSPayloadPAT();
    virtual ~TSPayloadPAT();
    int demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg);
};

class TSPMTESInfo
{
public:
    // 1B
    u_int8_t stream_type; //8bits

    // 2B
    // 3bits reserved
    int16_t elementary_PID; //13bits

    // 2B
    // 4bits reserved
    int16_t ES_info_length; //12bits

    char* ES_info; //[ES_info_length] bytes.

    TSPMTESInfo();
    virtual ~TSPMTESInfo();
};


/**
* 2.4.4.8 Program Map Table. page 64.
*/
class TSPayloadPMT
{
public:
    // 1B
    u_int8_t table_id; //8bits

    // 2B
    int8_t section_syntax_indicator; //1bit
    int8_t const0_value; //1bit
    // 2bits reserved.
    u_int16_t section_length; //12bits

    // 2B
    u_int16_t program_number; //16bits

    // 1B
    // 2bits reerverd.
    int8_t version_number; //5bits
    int8_t current_next_indicator; //1bit

    // 1B
    u_int8_t section_number; //8bits

    // 1B
    u_int8_t last_section_number; //8bits

    // 2B
    // 2bits reserved.
    int16_t PCR_PID; //16bits

    // 2B
    // 4bits reserved.
    int16_t program_info_length; //12bits
    char* program_info_desc; //[program_info_length]bytes

    // array of TSPMTESInfo.
    std::vector<TSPMTESInfo*> ES_info;

    // 4B
    int32_t CRC_32; //32bits

    TSPayloadPMT();
    virtual ~TSPayloadPMT();
    TSPMTESInfo* at(int index);
    int demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg);
};

/**
* 2.4.3.7 Semantic definition of fields in PES packet. page 49.
*/
class TSPayloadPES
{
public:
    // 3B
    int32_t packet_start_code_prefix; //24bits
    // 1B
    u_int8_t stream_id; //8bits
    // 2B
    u_int16_t PES_packet_length; //16bits

    // 1B
    // 2bits const '10'
    int8_t PES_scrambling_control; //2bits
    int8_t PES_priority; //1bit
    int8_t data_alignment_indicator; //1bit
    int8_t copyright; //1bit
    int8_t original_or_copy; //1bit

    // 1B
    int8_t PTS_DTS_flags; //2bits
    int8_t ESCR_flag; //1bit
    int8_t ES_rate_flag; //1bit
    int8_t DSM_trick_mode_flag; //1bit
    int8_t additional_copy_info_flag; //1bit
    int8_t PES_CRC_flag; //1bit
    int8_t PES_extension_flag; //1bit

    // 1B
    u_int8_t PES_header_data_length; //8bits

    int64_t pts; // 33bits
    int64_t dts; // 33bits

    int16_t ESCR_extension; //9bits
    int64_t ESCR_base; //33bits
    int32_t ES_rate; //22bits

    int8_t trick_mode_control; //3bits
    int8_t trick_mode_value; //5bits

    int8_t additional_copy_info; //7bits
    int16_t previous_PES_packet_CRC; //16bits

    int8_t PES_private_data_flag; //1bit
    int8_t pack_header_field_flag; //1bit
    int8_t program_packet_sequence_counter_flag; //1bit
    int8_t P_STD_buffer_flag; //1bit
    // reserved 3bits
    int8_t PES_extension_flag_2; //1bit

    // 16B
    char* PES_private_data; //128bits

    int8_t pack_field_length; //8bits
    char* pack_field; //[pack_field_length] bytes

    int8_t program_packet_sequence_counter; //7bits
    int8_t MPEG1_MPEG2_identifier; //1bit
    int8_t original_stuff_length; //6bits

    int8_t P_STD_buffer_scale; //1bit
    int16_t P_STD_buffer_size; //13bits

    int8_t PES_extension_field_length; //7bits
    char* PES_extension_field; //[PES_extension_field_length] bytes

    int stuffing_size;
    char* stuffing_byte;

    TSPayloadPES();
    virtual ~TSPayloadPES();
    int64_t decode_33bits_int(u_int8_t*& p, int64_t& temp);
    int64_t decode_33bits_int(int64_t& temp);
    int demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg);
};

class TSPayloadReserved
{
public:
    int size;
    char* bytes;

    TSPayloadReserved();
    virtual ~TSPayloadReserved();
    int demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg);
};


/**
* logic audio/video message
*/
class TSMessage
{
public:
    // 2.4.3.2 Transport Stream packet layer. page 36
    // the pid of PES packet.
    TSPidTable pid;

    // the type of pid.
    TSPidType type;
    // the type of stream, codec type.
    TSStreamType stream_type;
    // page 36
    u_int8_t continuity_counter;

    // 2.4.3.7 Semantic definition of fields in PES packet. page 49
    // PES packet header size plus data size.
    u_int16_t PES_packet_length; //16bits

    // the stream id.
    u_int8_t stream_id;

    // 2.4.3.7 Semantic definition of fields in PES packet. page 49.
    int32_t packet_start_code_prefix;

    int64_t pts; // 33bits
    int64_t dts; // 33bits
    int64_t pcr;

    // header size.
    int packet_header_size;

    // the parsed packet size.
    int parsed_packet_size;

    // total packet size.
    int packet_data_size;
    char* packet_data;

    // for avc.
    u_int8_t nal_ref_idc;
    u_int8_t nal_unit_type;

    TSMessage();
    virtual ~TSMessage();

    void append(u_int8_t*& p, int size);
    void detach(TSContext* ctx, TSMessage*& pmsg);

    bool is_video();
};

// ts context
class TSContext
{
public:
    /**
    * consumed pids.
    */
    int pid_size;
    TSPid* pids;
    int64_t ts_packet_count;
    std::map<TSPidTable, TSMessage*> msgs;

    TSContext();
    virtual ~TSContext();
    bool exists(TSPidTable pid);
    TSPid* get(TSPidTable pid);
    void push(TSPidTable pid, TSStreamType stream_type, TSPidType type, u_int8_t continuity_counter);

    TSMessage* get_msg(TSPidTable pid);
    void detach(TSMessage* msg);
};


#endif //PRACTICE_SERVER_ALL_TS_H
