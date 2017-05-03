//
// Created by root on 17-5-3.
//

#include "all_ts.h"


//---------TSContext
TSContext::TSContext()
{
    pid_size = 0;
    ts_packet_count = 0;
    pids = NULL;
}

TSContext::~TSContext()
{
    p_freep(pids);

    std::map<TSPidTable, TSMessage*>::iterator it;
    for (it = msgs.begin(); it != msgs.end(); ++it) {
        TSMessage* msg = it->second;
        p_freep(msg);
    }
    msgs.clear();
}

bool TSContext::exists(TSPidTable pid)
{
    for (int i = 0; i < pid_size; i++) {
        if (pid == pids[i].pid) {
            return true;
        }
    }

    return false;
}

TSPid* TSContext::get(TSPidTable pid)
{
    for (int i = 0; i < pid_size; i++) {
        if (pid == pids[i].pid) {
            return &pids[i];
        }
    }

    return NULL;
}

void TSContext::push(TSPidTable pid, TSStreamType stream_type, TSPidType type, u_int8_t continuity_counter)
{
    TSPid* p = get(pid);

    if (!p) {
        p = new TSPid[pid_size + 1];
        memcpy(p, pids, sizeof(TSPid) * pid_size);

        p[pid_size] = (TSPid){type, stream_type, pid, continuity_counter};
        pid_size++;

        p_freep(pids);
        pids = p;
    }

    p->continuity_counter = continuity_counter;
}


TSMessage* TSContext::get_msg(TSPidTable pid)
{
    if (msgs[pid] == NULL) {
        TSMessage* msg = new TSMessage();
        msg->pid = pid;
        msgs[pid] = msg;
    }

    return msgs[pid];
}

void TSContext::detach(TSMessage* msg)
{
    msgs[msg->pid] = NULL;
}

TSMessage::TSMessage()
{
    pid = TSPidTablePAT;
    type = TSPidTypeReserved;
    stream_type = TSStreamTypeReserved;
    stream_id = 0;
    packet_start_code_prefix = 0;
    pts = dts = pcr = 0;
    PES_packet_length = 0;
    packet_header_size = 0;
    parsed_packet_size = 0;
    packet_data_size = 0;
    packet_data = NULL;

    nal_ref_idc = 0;
    nal_unit_type = 0;
}

TSMessage::~TSMessage()
{
    p_freep(packet_data);
}

void TSMessage::append(u_int8_t*& p, int size)
{
    if (size <= 0) {
        return;
    }

    // for PES_packet_length is 0, the size is varient.
    if (packet_data_size - parsed_packet_size < size) {
        int realloc_size = size - (packet_data_size - parsed_packet_size);
        packet_data = (char*)realloc(packet_data, packet_data_size + realloc_size);

        packet_data_size += realloc_size;
    }

    memcpy(packet_data + parsed_packet_size, p, size);
    p += size;
    parsed_packet_size += size;
}

void TSMessage::detach(TSContext* ctx, TSMessage*& pmsg)
{
    if (parsed_packet_size >= packet_data_size) {
        ctx->detach(this);
        pmsg = this;
    }
}

bool TSMessage::is_video()
{
    return type == TSPidTypeVideo;
}

TSAdaptionField::TSAdaptionField()
{
    adaption_field_length = 0;
    discontinuity_indicator = 0;
    random_access_indicator = 0;
    elementary_stream_priority_indicator = 0;
    PCR_flag = 0;
    OPCR_flag = 0;
    splicing_point_flag = 0;
    transport_private_data_flag = 0;
    adaptation_field_extension_flag = 0;
    program_clock_reference_base = 0;
    program_clock_reference_extension = 0;
    original_program_clock_reference_base = 0;
    original_program_clock_reference_extension = 0;
    splice_countdown = 0;
    transport_private_data_length = 0;
    transport_private_data = NULL;
    adaptation_field_extension_length = 0;
    ltw_flag = 0;
    piecewise_rate_flag = 0;
    seamless_splice_flag = 0;
    ltw_valid_flag = 0;
    ltw_offset = 0;
    piecewise_rate = 0;
    splice_type = 0;
    DTS_next_AU0 = 0;
    marker_bit0 = 0;
    DTS_next_AU1 = 0;
    marker_bit1 = 0;
    DTS_next_AU2 = 0;
    marker_bit2 = 0;
    af_ext_reserved = NULL;
    af_reserved = NULL;
    __field_size = 0;
    pcr = 0;
    original_pcr = 0;
}

TSAdaptionField::~TSAdaptionField()
{
    p_freep(transport_private_data);
    p_freep(af_ext_reserved);
    p_freep(af_reserved);
}

int TSAdaptionField::get_size()
{
    return __field_size;
}

int TSAdaptionField::demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg)
{
    int ret = 0;

    adaption_field_length = *p++;
    u_int8_t* pos_af = p;
    __field_size = 1 + adaption_field_length;

    if (adaption_field_length <= 0) {
        trace("ts+af empty af decoded.");
        return ret;
    }

    int8_t value = *p++;

    discontinuity_indicator              =   (value >> 7) & 0x01;
    random_access_indicator              =   (value >> 6) & 0x01;
    elementary_stream_priority_indicator =   (value >> 5) & 0x01;
    PCR_flag                             =   (value >> 4) & 0x01;
    OPCR_flag                            =   (value >> 3) & 0x01;
    splicing_point_flag                  =   (value >> 2) & 0x01;
    transport_private_data_flag          =   (value >> 1) & 0x01;
    adaptation_field_extension_flag      =   (value >> 0) & 0x01;

    char* pp = NULL;
    if (PCR_flag) {
        pp = (char*)&program_clock_reference_base;
        pp[5] = *p++;
        pp[4] = *p++;
        pp[3] = *p++;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;

        program_clock_reference_extension = program_clock_reference_base & 0x1ff;
        program_clock_reference_base = (program_clock_reference_base >> 15) & 0x1ffffffffLL;

        // high 9bits
        pcr = program_clock_reference_extension;
        pcr = (pcr << 33) & 0x3fe00000000LL;
        // low 33bits
        pcr |= program_clock_reference_base;
    }
    if (OPCR_flag) {
        pp = (char*)&original_program_clock_reference_base;
        pp[5] = *p++;
        pp[4] = *p++;
        pp[3] = *p++;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;

        original_program_clock_reference_extension = original_program_clock_reference_base & 0x1ff;
        original_program_clock_reference_base = (original_program_clock_reference_base >> 15) & 0x1ffffffffLL;

        // high 9bits
        original_pcr = program_clock_reference_extension;
        original_pcr = (original_pcr << 33) & 0x3fe00000000LL;
        // low 33bits
        original_pcr |= program_clock_reference_base;
    }
    if (splicing_point_flag) {
        splice_countdown = *p++;
    }
    if (transport_private_data_flag) {
        transport_private_data_length = *p++;
        transport_private_data = new char[transport_private_data_length];
        for (int i = 0; i < transport_private_data_length; i++) {
            transport_private_data[i] = *p++;
        }
    }
    if (adaptation_field_extension_flag) {
        adaptation_field_extension_length = *p++;
        u_int8_t* pos_af_ext = p;

        ltw_flag = *p++;

        piecewise_rate_flag = (ltw_flag >> 6) & 0x01;
        seamless_splice_flag = (ltw_flag >> 5) & 0x01;
        ltw_flag = (ltw_flag >> 7) & 0x01;

        if (ltw_flag) {
            pp = (char*)&ltw_offset;
            pp[1] = *p++;
            pp[0] = *p++;

            ltw_valid_flag = (ltw_offset >> 15) &0x01;
            ltw_offset &= 0x7FFF;
        }
        if (piecewise_rate_flag) {
            pp = (char*)&piecewise_rate;
            pp[2] = *p++;
            pp[1] = *p++;
            pp[0] = *p++;

            piecewise_rate &= 0x3FFFFF;
        }
        if (seamless_splice_flag) {
            // 1B
            marker_bit0 = *p++;

            splice_type = (marker_bit0 >> 4) & 0x0F;
            DTS_next_AU0 = (marker_bit0 >> 1) & 0x07;
            marker_bit0 &= 0x01;

            // 2B
            pp = (char*)&DTS_next_AU1;
            pp[1] = *p++;
            pp[0] = *p++;

            marker_bit1 = DTS_next_AU1 & 0x01;
            DTS_next_AU1 = (DTS_next_AU1 >> 1) & 0x7FFF;

            // 2B
            pp = (char*)&DTS_next_AU2;
            pp[1] = *p++;
            pp[0] = *p++;

            marker_bit2 = DTS_next_AU2 & 0x01;
            DTS_next_AU2 = (DTS_next_AU2 >> 1) & 0x7FFF;
        }

        // af_ext_reserved
        int ext_size = adaptation_field_extension_length - (p - pos_af_ext);
        if (ext_size > 0) {
            af_ext_reserved = new char[ext_size];
            memcpy(af_ext_reserved, p, ext_size);
            p += ext_size;
        }
    }

    // af_reserved
    int af_size = adaption_field_length - (p - pos_af);
    if (af_size > 0) {
        af_reserved = new char[af_size];
        memcpy(af_reserved, p, af_size);
        p += af_size;
    }

    trace("ts+af af flags parsed, discontinuity: %d random: %d priority: %d PCR: %d OPCR: %d slicing: %d private: %d extension: %d pcr: %"PRId64" opcr: %"PRId64"",
            discontinuity_indicator, random_access_indicator, elementary_stream_priority_indicator, PCR_flag, OPCR_flag, splicing_point_flag,
            transport_private_data_flag, adaptation_field_extension_flag, pcr, original_pcr);

    return ret;
}

TSPayloadReserved::TSPayloadReserved()
{
    size = 0;
    bytes = NULL;
}

TSPayloadReserved::~TSPayloadReserved()
{
    p_freep(bytes);
}

int TSPayloadReserved::demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg)
{
    int ret = 0;

    size = pkt->payload->size - pkt->payload->pointer_field_size;

    // not parsed bytes.
    if (size > 0) {
        bytes = new char[size];
        memcpy(bytes, p, size);
        p += size;
    }

    return ret;
}

TSPayloadPAT::TSPayloadPAT()
{
    table_id = 0;
    section_syntax_indicator = 0;
    const0_value = 0;
    section_length = 0;
    transport_stream_id = 0;
    version_number = 0;
    current_next_indicator = 0;
    section_number = 0;
    last_section_number = 0;
    program_size = 0;
    programs = NULL;
    CRC_32 = 0;
}

TSPayloadPAT::~TSPayloadPAT()
{
    p_freep(programs);
}

int TSPayloadPAT::demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg)
{
    int ret = 0;

    table_id = *p++;

    char* pp = (char*)&section_length;
    pp[1] = *p++;
    pp[0] = *p++;
    u_int8_t* pos = p;

    section_syntax_indicator = (section_length >> 15) & 0x01;
    const0_value = (section_length >> 14) & 0x01;
    section_length &= 0x0FFF;

    pp = (char*)&transport_stream_id;
    pp[1] = *p++;
    pp[0] = *p++;

    current_next_indicator = *p++;
    version_number = (current_next_indicator >> 1) & 0x1F;
    current_next_indicator &= 0x01;

    section_number = *p++;
    last_section_number = *p++;

    // 4 is crc size.
    int program_bytes = section_length - 4 - (p - pos);
    program_size = program_bytes / 4;
    if (program_size > 0) {
        programs = new int32_t[program_size];
        for (int i = 0; i < program_size; i++) {
            pp = (char*)&programs[i];
            pp[3] = *p++;
            pp[2] = *p++;
            pp[1] = *p++;
            pp[0] = *p++;

            int16_t pid = programs[i] & 0x1FFF;
            ctx->push((TSPidTable)pid, TSStreamTypeReserved, TSPidTypePMT, pkt->header->continuity_counter);
        }
    }

    pp = (char*)&CRC_32;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;

    return ret;
}

TSPMTESInfo::TSPMTESInfo()
{
    stream_type = 0;
    elementary_PID = 0;
    ES_info_length = 0;
    ES_info = NULL;
}

TSPMTESInfo::~TSPMTESInfo()
{
    p_freep(ES_info);
}

TSPayloadPMT::TSPayloadPMT()
{
    table_id = 0;
    section_syntax_indicator = 0;
    const0_value = 0;
    section_length = 0;
    program_number = 0;
    version_number = 0;
    current_next_indicator = 0;
    section_number = 0;
    last_section_number = 0;
    PCR_PID = 0;
    program_info_length = 0;
    program_info_desc = NULL;
    CRC_32 = 0;
}

TSPayloadPMT::~TSPayloadPMT()
{
    p_freep(program_info_desc);

    for (std::vector<TSPMTESInfo*>::iterator it = ES_info.begin(); it != ES_info.end(); ++it) {
        TSPMTESInfo* info = *it;
        p_freep(info);
    }
    ES_info.clear();
}

TSPMTESInfo* TSPayloadPMT::at(int index)
{
    return ES_info.at(index);
}

int TSPayloadPMT::demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg)
{
    int ret = 0;

    table_id = *p++;

    char* pp = (char*)&section_length;
    pp[1] = *p++;
    pp[0] = *p++;
    u_int8_t* pos = p;

    section_syntax_indicator = (section_length >> 15) & 0x01;
    const0_value = (section_length >> 14) & 0x01;
    section_length &= 0x0FFF;

    pp = (char*)&program_number;
    pp[1] = *p++;
    pp[0] = *p++;

    current_next_indicator = *p++;
    version_number = (current_next_indicator >> 1) & 0x1F;
    current_next_indicator &= 0x01;

    section_number = *p++;
    last_section_number = *p++;

    pp = (char*)&PCR_PID;
    pp[1] = *p++;
    pp[0] = *p++;

    PCR_PID &= 0x1FFF;

    pp = (char*)&program_info_length;
    pp[1] = *p++;
    pp[0] = *p++;

    program_info_length &= 0xFFF;

    if (program_info_length > 0) {
        program_info_desc = new char[program_info_length];
        memcpy(program_info_desc, p, program_info_length);
        p += program_info_length;
    }

    // [section_length] - 4(CRC) - 9B - [program_info_length]
    int ES_bytes = section_length - 4 - 9 - program_info_length;
    while (ES_bytes > 0) {
        TSPMTESInfo* info = new TSPMTESInfo();

        info->stream_type = *p++;
        ES_bytes--;

        pp = (char*)&info->elementary_PID;
        pp[1] = *p++;
        pp[0] = *p++;
        ES_bytes -= 2;

        info->elementary_PID &= 0x1FFF;

        pp = (char*)&info->ES_info_length;
        pp[1] = *p++;
        pp[0] = *p++;
        ES_bytes -= 2;

        info->ES_info_length &= 0x0FFF;

        if (info->ES_info_length > 0) {
            info->ES_info = new char[info->ES_info_length];
            memcpy(info->ES_info, p, info->ES_info_length);

            p += info->ES_info_length;
            ES_bytes -= info->ES_info_length;
        }

        ES_info.push_back(info);

        if (info->stream_type == TSStreamTypeVideoH264) {
            // TODO: support more video type.
            ctx->push((TSPidTable)info->elementary_PID, (TSStreamType)info->stream_type, TSPidTypeVideo, pkt->header->continuity_counter);
            trace("ts+pmt add pid: %d, type: H264 video", info->elementary_PID);
        } else if (info->stream_type == TSStreamTypeAudioAAC) {
            // TODO: support more audio type.
            // see aac: 6.2 Audio Data Transport Stream, ADTS
            ctx->push((TSPidTable)info->elementary_PID, (TSStreamType)info->stream_type, TSPidTypeAudio, pkt->header->continuity_counter);
            trace("ts+pmt add pid: %d, type: AAC audio", info->elementary_PID);
        } else {
            trace("ts+pmt ignore the stream type: %d", info->stream_type);
        }
    }

    pp = (char*)&CRC_32;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;

    return ret;
}

TSPayloadPES::TSPayloadPES()
{
    packet_start_code_prefix = 0;
    stream_id = 0;
    PES_packet_length = 0;
    PES_scrambling_control = 0;
    PES_priority = 0;
    data_alignment_indicator = 0;
    copyright = 0;
    original_or_copy = 0;
    PTS_DTS_flags = 0;
    ESCR_flag = 0;
    ES_rate_flag = 0;
    DSM_trick_mode_flag = 0;
    additional_copy_info_flag = 0;
    PES_CRC_flag = 0;
    PES_extension_flag = 0;
    PES_header_data_length = 0;
    pts = 0;
    dts = 0;
    ESCR_extension = 0;
    ESCR_base = 0;
    ES_rate = 0;
    trick_mode_control = 0;
    trick_mode_value = 0;
    additional_copy_info = 0;
    previous_PES_packet_CRC = 0;
    PES_private_data_flag = 0;
    pack_header_field_flag = 0;
    program_packet_sequence_counter_flag = 0;
    P_STD_buffer_flag = 0;
    PES_extension_flag_2 = 0;
    PES_private_data = NULL;
    pack_field_length = 0;
    pack_field = NULL;
    program_packet_sequence_counter = 0;
    MPEG1_MPEG2_identifier = 0;
    original_stuff_length = 0;
    P_STD_buffer_scale = 0;
    P_STD_buffer_size = 0;
    PES_extension_field_length = 0;
    PES_extension_field = NULL;
    stuffing_size = 0;
    stuffing_byte = NULL;
}

TSPayloadPES::~TSPayloadPES()
{
    p_freep(PES_private_data);
    p_freep(pack_field);
    p_freep(PES_extension_field);
    p_freep(stuffing_byte);
}

int64_t TSPayloadPES::decode_33bits_int(u_int8_t*& p, int64_t& temp)
{
    char* pp = (char*)&temp;
    pp[4] = *p++;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;

    return decode_33bits_int(temp);
}

int64_t TSPayloadPES::decode_33bits_int(int64_t& temp)
{
    int64_t ret = 0;

    // marker_bit 1bit
    temp = temp >> 1;
    // PTS [14..0] 15bits
    ret |= temp & 0x7fff;
    // marker_bit 1bit
    temp = temp >> 1;
    // PTS [29..15] 15bits, 15zero, 29-15+1one
    ret |= temp & 0x3fff8000LL;
    // marker_bit 1bit
    temp = temp >> 1;
    // PTS [32..30] 3bits
    ret |= temp & 0x1c0000000LL;

    temp = temp >> 33;

    return ret;
}

int TSPayloadPES::demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg)
{
    int ret = 0;

    char* pp = (char*)&packet_start_code_prefix;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;

    packet_start_code_prefix &= 0xFFFFFF;
    if (packet_start_code_prefix != 0x01) {
        trace("ts+pes decode unit start packet error, msg is empty.");
        return -1;
    }

    stream_id = *p++;

    pp = (char*)&PES_packet_length;
    pp[1] = *p++;
    pp[0] = *p++;
    u_int8_t* pos_packet = p;

    if (stream_id != PES_program_stream_map
        && stream_id != PES_padding_stream
        && stream_id != PES_private_stream_2
        && stream_id != PES_ECM_stream
        && stream_id != PES_EMM_stream
        && stream_id != PES_program_stream_directory
        && stream_id != PES_DSMCC_stream
        && stream_id != PES_H_222_1_type_E
            ) {
        original_or_copy = *p++;

        //int8_t const2bits = (original_or_copy >> 6) & 0x03;
        PES_scrambling_control = (original_or_copy >> 4) & 0x03;
        PES_priority = (original_or_copy >> 3) & 0x01;
        data_alignment_indicator = (original_or_copy >> 2) & 0x01;
        copyright = (original_or_copy >> 1) & 0x01;
        original_or_copy &= 0x01;

        PES_extension_flag = *p++;

        PTS_DTS_flags = (PES_extension_flag >> 6) & 0x03;
        ESCR_flag = (PES_extension_flag >> 5) & 0x01;
        ES_rate_flag = (PES_extension_flag >> 4) & 0x01;
        DSM_trick_mode_flag = (PES_extension_flag >> 3) & 0x01;
        additional_copy_info_flag = (PES_extension_flag >> 2) & 0x01;
        PES_CRC_flag = (PES_extension_flag >> 1) & 0x01;
        PES_extension_flag &= 0x01;

        PES_header_data_length = *p++;
        u_int8_t* pos_header = p;

        int64_t temp = 0;
        if (PTS_DTS_flags == 0x2) {
            pts = decode_33bits_int(p, temp);
            // '0010' 4bits
            //int8_t const4bits = temp & 0x0F;
        }

        if (PTS_DTS_flags == 0x3) {
            pts = decode_33bits_int(p, temp);
            // '0011' 4bits
            //int8_t const4bits = temp & 0x0F;

            dts = decode_33bits_int(p, temp);
            // '0001' 4bits
            //int8_t const4bits = temp & 0x0F;
        }

        if (ESCR_flag) {
            pp = (char*)&temp;
            pp[5] = *p++;
            pp[4] = *p++;
            pp[3] = *p++;
            pp[2] = *p++;
            pp[1] = *p++;
            pp[0] = *p++;

            // marker_bit 1bit
            temp = temp >> 1;
            // ESCR_extension 9bits
            ESCR_extension = temp & 0x1f;
            temp = temp >> 9;

            ESCR_base = decode_33bits_int(temp);

            // reserved 2bits
            //int8_t reserved2bits = temp & 0x03;
        }

        if (ES_rate_flag) {
            pp = (char*)&ES_rate;
            pp[2] = *p++;
            pp[1] = *p++;
            pp[0] = *p++;

            ES_rate = ES_rate >> 1;
            ES_rate &= 0x3FFFFF;
        }

        if (DSM_trick_mode_flag) {
            trick_mode_control = *p++;

            trick_mode_value = trick_mode_control & 0x1f;
            trick_mode_control = (trick_mode_control >> 5) & 0x03;
        }

        if (additional_copy_info_flag) {
            additional_copy_info = *p++;
            additional_copy_info &= 0x7f;
        }

        if (PES_CRC_flag) {
            pp = (char*)&previous_PES_packet_CRC;
            pp[1] = *p++;
            pp[0] = *p++;
        }

        if (PES_extension_flag) {
            PES_extension_flag_2 = *p++;

            PES_private_data_flag = (PES_extension_flag_2 >> 7) & 0x01;
            pack_header_field_flag = (PES_extension_flag_2 >> 6) & 0x01;
            program_packet_sequence_counter_flag = (PES_extension_flag_2 >> 5) & 0x01;
            P_STD_buffer_flag = (PES_extension_flag_2 >> 4) & 0x01;
            PES_extension_flag_2 &= PES_extension_flag_2 & 0x01;

            if (PES_private_data_flag) {
                PES_private_data = new char[16];
                memcpy(PES_private_data, p, 16);
                p += 16;
            }

            if (pack_header_field_flag) {
                pack_field_length = *p++;
                if (pack_field_length > 0) {
                    pack_field = new char[pack_field_length];
                    memcpy(pack_field, p, pack_field_length);
                    p += pack_field_length;
                }
            }

            if (program_packet_sequence_counter_flag) {
                program_packet_sequence_counter = *p++;
                program_packet_sequence_counter &= 0x7f;

                original_stuff_length = *p++;
                MPEG1_MPEG2_identifier = (original_stuff_length >> 6) & 0x01;
                original_stuff_length &= 0x3f;
            }

            if (P_STD_buffer_flag) {
                pp = (char*)&P_STD_buffer_size;
                pp[1] = *p++;
                pp[0] = *p++;

                // '01'
                //int8_t const2bits = (P_STD_buffer_scale >>14) & 0x03;

                P_STD_buffer_scale = (P_STD_buffer_scale >>13) & 0x01;
                P_STD_buffer_size &= 0x1FFF;
            }

            if (PES_extension_flag_2) {
                PES_extension_field_length = *p++;
                PES_extension_field_length &= 0x07;

                if (PES_extension_field_length > 0) {
                    PES_extension_field = new char[PES_extension_field_length];
                    memcpy(PES_extension_field, p, PES_extension_field_length);
                    p += PES_extension_field_length;
                }
            }
        }

        // stuffing_byte
        int stuffing_size = PES_header_data_length - (p - pos_header);
        if (stuffing_size > 0) {
            stuffing_byte = new char[stuffing_size];
            memcpy(stuffing_byte, p, stuffing_size);
            p += stuffing_size;
        }

        // get the pid.
        TSPid* pid = ctx->get(pkt->header->pid);
        if (!pid) {
            trace("ts+pes pid: %d type is invalid.", pkt->header->pid);
        }

        // get the message to build from the chunks(PES packets).
        TSMessage* msg = ctx->get_msg(pid->pid);

        msg->type = pid->type;
        msg->stream_type = pid->stream_type;
        msg->continuity_counter = pid->continuity_counter;
        msg->stream_id = stream_id;
        msg->packet_start_code_prefix = packet_start_code_prefix;
        msg->dts = dts;
        msg->pts = pts;

        // PES_packet_data_byte, page58.
        // the packet size contains the header size.
        // The number of PES_packet_data_bytes, N, is specified by the
        // PES_packet_length field. N shall be equal to the value
        // indicated in the PES_packet_length minus the number of bytes
        // between the last byte of the PES_packet_length field and the
        // first PES_packet_data_byte.
        msg->PES_packet_length = PES_packet_length;
        msg->packet_header_size = p - pos_packet;
        msg->packet_data_size = PES_packet_length - msg->packet_header_size;

        /**
        * when actual packet length > 0xffff(65535),
        * which exceed the max u_int16_t packet length,
        * use 0 packet length, the next unit start indicates the end of packet.
        */
        if (PES_packet_length == 0) {
            msg->packet_data_size = last - p - msg->packet_header_size;
        }

        if (msg->packet_data_size > 0) {
            msg->packet_data = new char[msg->packet_data_size];
        }

        // PES_packet_data_byte
        int size = p_min(msg->packet_data_size, last - p);
        if (size > 0) {
            msg->append(p, size);
        }

        if (PES_packet_length > 0) {
            msg->detach(ctx, pmsg);
        }

        trace("ts+pes stream_id: %d size: %d pts: %"PRId64" dts: %"PRId64" total: %d header: %d packet_size: %d parsed_size: %d",
                stream_id, PES_packet_length, pts, dts, msg->PES_packet_length, msg->packet_header_size, msg->packet_data_size, msg->parsed_packet_size);
    } else if (stream_id == PES_program_stream_map
               || stream_id == PES_private_stream_2
               || stream_id == PES_ECM_stream
               || stream_id == PES_EMM_stream
               || stream_id == PES_program_stream_directory
               || stream_id == PES_DSMCC_stream
               || stream_id == PES_H_222_1_type_E
            ) {
        // for (i = 0; i < PES_packet_length; i++) {
        //         PES_packet_data_byte
        // }
        // TODO: FIXME: implements it.
    } else if (stream_id == PES_padding_stream) {
        // for (i = 0; i < PES_packet_length; i++) {
        //         padding_byte
        // }
    }

    return ret;
}

/**
* 2.4.3.6 PES packet. page 49.
*/

TSPayload::TSPayload()
{
    size = 0;
    pointer_field_size = 0;
    type = TSPidTypeReserved;
    pointer_field = 0;
    reserved = NULL;
    pat = NULL;
    pmt = NULL;
    pes = NULL;

}

TSPayload::~TSPayload()
{
    p_freep(reserved);
    p_freep(pat);
    p_freep(pmt);
    p_freep(pes);
}

void TSPayload::read_pointer_field(TSPacket* pkt, u_int8_t*& p)
{
    if (pkt->header->payload_unit_start_indicator) {
        pointer_field = *p++;
        pointer_field_size = 1;
    }
}

int TSPayload::demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg)
{
    int ret = 0;

    if (pkt->header->pid == TSPidTablePAT) {
        read_pointer_field(pkt, p);

        type = TSPidTypePAT;
        pat = new TSPayloadPAT();
        return pat->demux(ctx, pkt, start, last, p, pmsg);
    }

    TSPid* pid = ctx->get(pkt->header->pid);
    if (pid && pid->type == TSPidTypePMT) {
        read_pointer_field(pkt, p);

        type = pid->type;
        pmt = new TSPayloadPMT();
        return pmt->demux(ctx, pkt, start, last, p, pmsg);
    }
    if (pid && (pid->type == TSPidTypeVideo || pid->type == TSPidTypeAudio)) {
        TSMessage* msg = ctx->get_msg(pkt->header->pid);

        if (pkt->adaption_field->pcr > 0) {
            msg->pcr = pkt->adaption_field->pcr;
        }

        // flush previous PES_packet_length(0) packets.
        if (msg->packet_start_code_prefix == 0x01
            && pkt->header->payload_unit_start_indicator == 1
            && msg->PES_packet_length == 0
                ) {
            msg->detach(ctx, pmsg);
            // reparse current message
            p = start;
            return ret;
        }

        // parse continous packet.
        if (!pkt->header->payload_unit_start_indicator) {
            if (msg->packet_start_code_prefix != 0x01) {
                trace("ts+pes decode continous packet error, msg is empty.");
                return -1;
            }
            msg->append(p, last - p);

            // for PES_packet_length is 0, donot attach it.
            if (msg->PES_packet_length > 0) {
                msg->detach(ctx, pmsg);
            }
            return ret;
        }

        type = pid->type;
        pes = new TSPayloadPES();
        return pes->demux(ctx, pkt, start, last, p, pmsg);
    }

    // not parsed bytes.
    type = TSPidTypeReserved;
    reserved = new TSPayloadReserved();
    if ((ret = reserved->demux(ctx, pkt, start, last, p, pmsg)) != 0) {
        return ret;
    }

    return ret;
}

TSPacket::TSPacket()
{
    header = new TSHeader();
    adaption_field = new TSAdaptionField();
    payload = new TSPayload();
}

TSPacket::~TSPacket()
{
    p_freep(header);
    p_freep(adaption_field);
    p_freep(payload);
}

int TSPacket::demux(TSContext* ctx, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg)
{
    int ret = 0;

    if ((ret = header->demux(ctx, this, start, last, p, pmsg))  != 0) {
        return ret;
    }

    if (header->adaption_field_control == TSAdaptionTypeAdaptionOnly || header->adaption_field_control == TSAdaptionTypeBoth) {
        if ((ret = adaption_field->demux(ctx, this, start, last, p, pmsg)) != 0) {
            trace("ts+header af(adaption field) decode error. ret=%d", ret);
            return ret;
        }
        trace("ts+header af(adaption field) decoded.");
    }

    // calc the user defined data size for payload.
    payload->size = TS_PACKET_SIZE - header->get_size() - adaption_field->get_size();

    if (header->adaption_field_control == TSAdaptionTypePayloadOnly || header->adaption_field_control == TSAdaptionTypeBoth) {
        // parse new packet.
        if ((ret = payload->demux(ctx, this, start, last, p, pmsg)) != 0) {
            trace("ts+header payload decode error. ret=%d", ret);
            return ret;
        }
        trace("ts+header payload decoded.");
    }

    ctx->ts_packet_count++;
    trace("ts+header parsed %"PRId64" packets finished. parsed: %d left: %d header: %d payload: %d(%d+%d)",
            ctx->ts_packet_count, (int)(p - start), (int)(last - p), header->get_size(), payload->size, payload->pointer_field_size,
            payload->size - payload->pointer_field_size);

    return finish();
}

int TSPacket::finish()
{
    return 0;
}

TSHeader::TSHeader()
{
    sync_byte = 0;
    transport_error_indicator = 0;
    payload_unit_start_indicator = 0;
    transport_priority = 0;
    pid = TSPidTablePAT;
    transport_scrambling_control = 0;
    adaption_field_control = TSAdaptionTypeReserved;
    continuity_counter = 0;
}

TSHeader::~TSHeader()
{
}

int TSHeader::get_size()
{
    return 4;
}

int TSHeader::demux(TSContext* ctx, TSPacket* pkt, u_int8_t* start, u_int8_t* last, u_int8_t*& p, TSMessage*& pmsg)
{
    int ret = 0;

    // ts packet header.
    sync_byte = *p++;
    if (sync_byte != 0x47) {
        trace("ts+sync_bytes invalid sync_bytes: %#x, expect is 0x47", sync_byte);
        return -1;
    }

    int16_t _pid = 0;
    char* pp = (char*)&_pid;
    pp[1] = *p++;
    pp[0] = *p++;

    transport_error_indicator = (_pid >> 15) & 0x01;
    payload_unit_start_indicator = (_pid >> 14) & 0x01;
    transport_priority = (_pid >> 13) & 0x01;
    _pid &= 0x1FFF;

    pid = (TSPidTable)_pid;

    continuity_counter = *p++;

    transport_scrambling_control = (continuity_counter >> 6) & 0x03;
    int8_t _adaption_field_control = (continuity_counter >> 4) & 0x03;
    adaption_field_control = (TSAdaptionType)_adaption_field_control;
    continuity_counter &= 0x0F;

    ctx->push(pid, TSStreamTypeReserved, TSPidTypePAT, continuity_counter);

    trace("ts+header sync: %#x error: %d unit_start: %d priotiry: %d pid: %d scrambling: %d adaption: %d counter: %d",
          sync_byte, transport_error_indicator, payload_unit_start_indicator, transport_priority, pid,
          transport_scrambling_control, adaption_field_control, continuity_counter);

    return ret;
}

/**
* Annex B Byte stream format, in page 211.
*/
class TSH264Codec
{
public:
    u_int8_t* raw_data;
    int size;

    TSH264Codec()
    {
        size = 0;
        raw_data = NULL;
    }

    u_int8_t at(int index)
    {
        if (index >= size) {
            return 0;
        }
        return raw_data[index];
    }

    int parse(TSMessage* msg, char* last, char*& p)
    {
        int ret = 0;

        p_assert(p);

        while (next_start_code_prefix(p) != 0x000001) {
            char ch = *p++;
            if (ch != 0x00) {
                trace("ts+h264 parse msg failed, "
                              "expect 0x00 before start-code. actual is: %#x", (u_int8_t)ch);
                return -1;
            }
        }

        if (p >= last) {
            trace("ts+h264 parse msg finished, no start-code.");
            return ret;
        }

        // start_code_prefix_one_3bytes /* equal to 0x000001 */
        p += 3;

        if (p < last) {
            raw_data = (u_int8_t*)p;
        }
        while (p < last - 3) {
            if (match_start_code_prefix(p)) {
                break;
            }
            p++;
        }

        if (raw_data) {
            size = (u_int8_t*)p - raw_data;
            if (p == last - 3) {
                size = (u_int8_t*)last - raw_data;
                p = last;
            }
        }

        trace("ts+h264 parse msg finished");
        return ret;
    }

    bool match_start_code_prefix(char*p)
    {
        return p[0] == 0x00 && p[1] == 0x00 && (p[2] == 0x00 || p[2] == 0x01);
    }

    int32_t next_start_code_prefix(char* p)
    {
        int32_t value = 0;
        char* pp = (char*)&value;

        pp[2] = p[0];
        pp[1] = p[1];
        pp[0] = p[2];

        return value;
    }
};



/**
* 6.2 Audio Data Transport Stream, ADTS, in page 26.
*/
class TSAacAdts
{
public:
    // adts_fixed_header
    // 2B, 16bits
    int16_t syncword; //12bits
    int8_t ID; //1bit
    int8_t layer; //2bits
    int8_t protection_absent; //1bit
    // 12bits
    int8_t profile; //2bit
    TSAacSampleFrequency sampling_frequency_index; //4bits
    int8_t private_bit; //1bit
    int8_t channel_configuration; //3bits
    int8_t original_or_copy; //1bit
    int8_t home; //1bit

    // adts_variable_header
    // 28bits
    int8_t copyright_identification_bit; //1bit
    int8_t copyright_identification_start; //1bit
    int16_t frame_length; //13bits
    int16_t adts_buffer_fullness; //11bits
    int8_t number_of_raw_data_blocks_in_frame; //2bits

    u_int8_t* raw_data;
    int size;

    TSAacAdts()
    {
        syncword = 0;
        ID = 0;
        layer = 0;
        protection_absent = 0;
        profile = 0;
        sampling_frequency_index = TSAacSampleFrequencyReserved0;
        private_bit = 0;
        channel_configuration = 0;
        original_or_copy = 0;
        home = 0;
        copyright_identification_bit = 0;
        copyright_identification_start = 0;
        frame_length = 0;
        adts_buffer_fullness = 0;
        number_of_raw_data_blocks_in_frame = 0;

        size = 0;
        raw_data = NULL;
    }

    u_int8_t at(int index)
    {
        if (index >= size) {
            return 0;
        }
        return raw_data[index];
    }

    int parse(TSMessage* msg, char*& p)
    {
        int ret = 0;

        p_assert(p);

        char* start = p;

        // adts_fixed_header
        char* pp = (char*)&syncword;
        pp[1] = *p++;
        pp[0] = *p++;

        protection_absent = syncword & 0x01;
        layer = (syncword >> 1) & 0x03;
        ID = (syncword >> 3) & 0x01;
        syncword = (syncword >> 4) & 0x0FFF;
        if (syncword != 0xfff) {
            trace("ts+aac invalid sync word. expect 0xfff, actual %#x", syncword);
            return -1;
        }

        // adts_variable_header
        int64_t temp = 0;
        pp = (char*)&temp;
        pp[4] = *p++;
        pp[3] = *p++;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;

        number_of_raw_data_blocks_in_frame = temp & 0x03;
        temp = temp >> 2;

        adts_buffer_fullness = temp & 0x7FF;
        temp = temp >> 11;

        frame_length = temp & 0x1FFF;
        temp = temp >> 13;

        copyright_identification_start = temp & 0x01;
        temp = temp >> 1;

        copyright_identification_bit = temp & 0x01;
        temp = temp >> 1;

        // adts_fixed_header
        home = temp & 0x01;
        temp = temp >> 1;

        original_or_copy = temp & 0x01;
        temp = temp >> 1;

        channel_configuration = temp & 0x07;
        temp = temp >> 3;

        private_bit = temp & 0x01;
        temp = temp >> 1;

        sampling_frequency_index = (TSAacSampleFrequency)(temp & 0x0F);
        temp = temp >> 4;

        profile = temp & 0x03;
        temp = temp >> 2;

        if (!number_of_raw_data_blocks_in_frame) {
            // adts_error_check
            if (!protection_absent) {
                // crc_check
                trace("ts+aac TODO: crc_check.");
            }
            // raw_data_block
            raw_data = (u_int8_t*)p;
            size = frame_length - (p - start);
            p += size;
        } else {
            trace("ts+aac TODO: parse multiple blocks.");
        }

        return ret;
    }
};

class AacMuxer
{
public:
    int fd;
    const char* file;

    AacMuxer()
    {
        file = NULL;
        fd = 0;
    }

    virtual ~AacMuxer()
    {
        if (fd > 0) {
            close(fd);
        }
    }

    int open(const char* _file)
    {
        file = _file;
        if ((fd = ::open(file, O_CREAT|O_WRONLY|O_TRUNC,
                         S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH)) < 0
                ) {
            return -1;
        }

        return 0;
    }

    int write_audio(char* data, int size)
    {
        if (size > 0 && write(fd, data, size) != size) {
            return -1;
        }

        return 0;
    }

    int write_video(char* data, int size)
    {
        return 0;
    }
};

int consume(TSMessage* msg, AacMuxer* aac_muxer)
{
    int ret = 0;

    char* p = msg->packet_data;
    if (!p) {
        trace("ts+aac+h264 ignore empty message.");
        return ret;
    }

    char* last = msg->packet_data + msg->packet_data_size;

    if (!msg->is_video()) {
        // write AAC raw audio.
        if (aac_muxer && (ret = aac_muxer->write_audio((char*)msg->packet_data, msg->packet_data_size)) != 0) {
            return ret;
        }

        // parse AAC audio.
        int64_t dts = -1;
        while (p < last) {
            TSAacAdts aac;
            if ((ret = aac.parse(msg, p)) != 0) {
                return ret;
            }
            trace("ts+aac audio raw data parsed, size: %d, 0x%02x 0x%02x 0x%02x 0x%02x",
                  aac.size, aac.at(0), aac.at(1), aac.at(2), aac.at(3));

            if (dts == -1) {
                dts = (msg->dts == 0)? msg->pts : msg->dts;
            } else {
                // see ffmpeg: avpriv_aac_parse_header
                // rdb = get_bits(gbc, 2);  /* number_of_raw_data_blocks_in_frame */
                // hdr->samples  = (rdb + 1) * 1024;
                int samples = (aac.number_of_raw_data_blocks_in_frame + 1) * 1024;
                static int sample_rates[] = {
                        96000, 88200, 64000, 48000, 44100, 32000,
                        24000, 22050, 16000, 12000, 11025, 8000,
                        1, 1, 1, 1
                };
                int sample_rate = sample_rates[aac.sampling_frequency_index];

                dts += samples * 90000 / sample_rate;
            }

            trace("ts+aac+h264+data %s pts:%"PRId64" dts:%"PRId64" size: %d",
                    (msg->type == TSPidTypeVideo)? "video":"audio", dts, dts, aac.frame_length);

            // TODO: process audio.
        }
    } else {
        trace("ts+aac+h264+data %s pts:%"PRId64" dts:%"PRId64" size: %d",
                (msg->type == TSPidTypeVideo)? "video":"audio", msg->pts,
                (msg->dts == 0)? msg->pts : msg->dts, msg->packet_data_size);

        // parse H264 video.
        bool first = true;
        while (p < last) {
            TSH264Codec h264;
            if ((ret = h264.parse(msg, last, p)) != 0) {
                return ret;
            }
            trace("ts+h264 video raw data parsed, size: %d, 0x%02x 0x%02x 0x%02x 0x%02x",
                  h264.size, h264.at(0), h264.at(1), h264.at(2), h264.at(3));

            // first?
            if (!first) {
                continue;
            }
            first = false;

            // TODO: process video.

            // directly check the sequence header for test_22m.flv
            if (h264.at(0) == 0x67 && h264.at(1) == 0x00 && h264.at(2) == 0x1f && h264.at(3) == 0xac) {
                trace("ts+h264 directly find the sequence header for test_22m.flv");
            }
            // 7.3.1 NAL unit syntax, hls-mpeg-ts-iso13818-1.pdf, page 44
            char* pp = (char*)h264.raw_data;
            int8_t nal_unit_type = *pp++;
            int8_t nal_ref_idc = (nal_unit_type >> 5) & 0x03;
            nal_unit_type &= 0x1f;

            msg->nal_ref_idc = nal_ref_idc;
            msg->nal_unit_type = nal_unit_type;

            if (nal_ref_idc != 0) {
                trace("ts+h264 got an SPS or PPS.");
            }
            if (nal_unit_type == 7) {
                trace("ts+h264 got an SPS.");
            } else if (nal_unit_type == 5) {
                trace("ts+h264 got an Coded slice of an IDR picture.");
            } else if (nal_unit_type == 8) {
                trace("ts+h264 got an PPS.");
            } else if (nal_unit_type == 9) {
                trace("ts+h264 got an Picture delimiter.");
                int8_t pic_type = *pp++;
                pic_type = (pic_type >> 6) & 0x07;
                if (pic_type == 0) {
                    trace("ts+h264 got an I picture.");
                } else if (pic_type == 1) {
                    trace("ts+h264 got an I,P picture.");
                } else if (pic_type == 2) {
                    trace("ts+h264 got an I,P,B picture.");
                } else if (pic_type == 3) {
                    trace("ts+h264 got an SI picture.");
                } else if (pic_type == 4) {
                    trace("ts+h264 got an SI,SP picture.");
                } else if (pic_type == 5) {
                    trace("ts+h264 got an I,SI picture.");
                } else if (pic_type == 6) {
                    trace("ts+h264 got an I,SI,P,SP picture.");
                } else if (pic_type == 7) {
                    trace("ts+h264 got an I,SI,P,SP,B picture.");
                }
            } else {
                trace("ts+h264 got an unknown unit type: %d.", nal_unit_type);
            }
        }
    }

    return ret;
}

int main(int argc, char** argv)
{
    const char* file = "livestream-1347.ts";
    const char* output_aac_file = "output.aac";
    if (argc > 2) {
        file = argv[1];
        output_aac_file = argv[2];
    }

    int fd = open(file, O_RDONLY);
    AacMuxer aac_muxer;

    int ret = 0;
    if ((ret = aac_muxer.open(output_aac_file)) != 0) {
        trace("aac_muxer+open open flv file failed.");
        return ret;
    }

    trace("demuxer+read packet count offset T+0  T+1  T+2  T+3  T+x T+L2 T+L1 T+L0");

    TSContext ctx;
    for (int i = 0, offset = 0; ; i++) {
        u_int8_t ts_packet[TS_PACKET_SIZE];
        memset(ts_packet, 0, sizeof(ts_packet));

        int nread = read(fd, ts_packet, sizeof(ts_packet));
        if (nread == 0) {
            trace("demuxer+read got EOF, read completed, offset: %07d.", offset);
            break;
        }
        if (nread != TS_PACKET_SIZE) {
            trace("demuxer+read error to read ts packet. nread=%d", nread);
            break;
        }
        trace("demuxer+read packet %04d %07d 0x%02x 0x%02x 0x%02x 0x%02x ... 0x%02x 0x%02x 0x%02x",
              i, offset, ts_packet[0], ts_packet[1], ts_packet[2], ts_packet[3],
              ts_packet[TS_PACKET_SIZE - 3], ts_packet[TS_PACKET_SIZE - 2], ts_packet[TS_PACKET_SIZE - 1]);

        u_int8_t* p = ts_packet;
        u_int8_t* start = ts_packet;
        u_int8_t* last = ts_packet + TS_PACKET_SIZE;

        // maybe need to parse multiple times for the PES_packet_length(0) packets.
        while (p == start) {
            TSPacket pkt;
            TSMessage* msg = NULL;
            if ((ret = pkt.demux(&ctx, start, last, p, msg)) != 0) {
                trace("demuxer+read decode ts packet error. ret=%d", ret);
                return ret;
            }

            offset += nread;
            if (!msg) {
                continue;
            }

            if ((ret = consume(msg, &aac_muxer)) != 0) {
                trace("demuxer+consume parse and consume message failed. ret=%d", ret);
                return -1;
            }

            int64_t pts = msg->pts;
            int64_t dts = (msg->dts == 0)? msg->pts : msg->dts;
            int64_t pcr = msg->pcr;
            static int64_t last_pcr_dts = 0;
            trace("demuxer+report id=%d, type=%s, size=%d, dts=%d, pts=%d, cts=%d, pcr=%d, dts-pcr=%d, ref=%d, unit=%d, dts(diff-pcr)=%d",
                  (int)ctx.ts_packet_count, (msg->type == TSPidTypeVideo)? "video":"audio",
                  (int)msg->parsed_packet_size, (int)dts, (int)pts, (int)(pts - dts), (int)pcr, (int)(pcr? dts - pcr : 0),
                  (int)msg->nal_ref_idc, (int)msg->nal_unit_type, (int)(pcr? dts - last_pcr_dts: 0));
            if (pcr > 0) {
                last_pcr_dts = dts;
            }

            p_freep(msg);
        }
    }

    close(fd);
    return ret;
}
