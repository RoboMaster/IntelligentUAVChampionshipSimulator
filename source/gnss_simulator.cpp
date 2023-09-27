#include "gnss_simulator.h"
#include <cmath>
#include <functional>

GNSSSimulator::GNSSSimulator(const std::string& config_string):Simulator()
{   
    m_run_thread_exit = true;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( config_string.c_str(), m_root );  
    if ( !parsingSuccessful )
    {
        LOG(FATAL)  << "Failed to parse" << reader.getFormattedErrorMessages();
    }
    m_status = Status::pending;
    m_results["map"] = 1;
    m_results["status"] = "pending";
    Json::Value monitor;
    monitor["status"] = "normal";
    monitor["enter"] = "";  
    monitor["ouput"] = "";
    m_results["monitor"] = monitor;
    Json::Value result_inner;
    result_inner["score"] = 0.0;
    result_inner["status"] = "";
    result_inner["positionAccuracy"] = 0.0;
    result_inner["offset"] = 0.0;
    result_inner["speedAccuracy"] = 0.0;
    result_inner["availability"] = 0.0;
    result_inner["resultPath"] = "";
    result_inner["resultMD5"] = "";
    result_inner["recordPath"] = "";
    result_inner["recordMD5"] = "";
    m_results["results"] = result_inner;

    std::fstream f(m_root["Data"]["GroundTruePath"].asString(), std::ios::in);
    if(!f.is_open())
    {
        LOG(FATAL)<<"Can not open gt file";
    }
    std::string sa, ssa;
    std::array<double, 7> tmp_data;
    while(std::getline(f, sa))
    {
        int dex = 0;
        int end = sa.find(','); 
        std::stringstream tmp_ss;
        while (end != -1) { // Loop until no delimiter is left in the string.
            // std::cout <<"origin: "<< sa.substr(0, end) << std::endl;
            ssa = sa.substr(0, end);
            tmp_ss << ssa;
            tmp_ss >> std::setprecision(16) >> tmp_data[dex]; 
            tmp_ss.clear();
            // std::cout << "convert: "<<tmp_data[dex] << std::endl;
            dex += 1;
            sa.erase(sa.begin(), sa.begin() + end + 1);
            end = sa.find(',');
        }
        // std::cout << sa.substr(0, end);
        // std::cout<<"\n---"<<std::endl;
        ssa = sa.substr(0, end);
        tmp_data[dex] = std::stof(ssa);
        m_ground_true.emplace_back(tmp_data);
    }
    // for(int i = 1; i < m_ground_true.size(); i++)
    // {
    //     // if(std::fabs(m_ground_true[i][0]-m_ground_true[i-1][0] - 0.2)>0.0001)std::cout<<"Idx: "<<i<<" "<<m_ground_true[i][0]-m_ground_true[i-1][0]<<std::endl;
    //     std::cout<<"Index: "<<i<<std::endl;
    //     std::cout<<m_ground_true[i][0]<<std::endl;
    //     std::cout<<m_ground_true[i][1]<<std::endl;
    //     std::cout<<m_ground_true[i][2]<<std::endl;
    //     std::cout<<m_ground_true[i][3]<<std::endl;
    //     std::cout<<m_ground_true[i][4]<<std::endl;
    //     std::cout<<m_ground_true[i][5]<<std::endl;
    //     std::cout<<m_ground_true[i][6]<<std::endl;
    // }
    m_final_score = 0;
    m_max_error_score_p = 0;
    m_max_error_score_v = 0;
    m_scores_p.clear();
    m_scores_v.clear();
    m_scores_p_vector.clear();
    m_scores_v_vector.clear();
    m_scores_p_gt_vector.clear();
    m_scores_v_gt_vector.clear();
    m_save_data_per_second = m_root["Command"]["SaveDataPerSecond"].asBool();
    m_start_timestamp = 0;

    m_curr_data_idx = 0;
    m_curr_buffer_size = 0;
    m_curr_data_block_offset = 0;

    m_shutdown_flag = false;
    std::set<std::filesystem::path> data_files_set;
    LOG(INFO)<<"Data path: "<<m_root["Data"]["FolderPath"].asString();
    for (auto &entry : std::filesystem::directory_iterator(m_root["Data"]["FolderPath"].asString()))data_files_set.insert(entry.path());
    for (auto &filename : data_files_set)m_data_files_vec.push_back(filename);
    m_data_num = m_data_files_vec.size();
    m_sending_data_size_per_second = m_root["Data"]["SendSize"].asInt();
    m_data_block_size = m_root["Data"]["BlockSize"].asInt();
    m_send_freq = m_root["Data"]["SendFreq"].asFloat();
    m_packet_size = m_root["Data"]["PacketSize"].asInt();
    m_buffer_ptr = new char[m_sending_data_size_per_second];
    m_tmp_buffer_ptr = new char[m_data_block_size];

    m_socket_handler = socket(AF_INET, SOCK_DGRAM, 0);
    if(m_socket_handler==-1)LOG(FATAL)<<"Can not create socket";
    bzero(&m_sock_ser, (uint)sizeof(m_sock_ser));
    m_sock_ser.sin_family = AF_INET;
    m_sock_ser.sin_addr.s_addr = inet_addr(m_root["UDPServer"]["IP"].asCString());
    m_sock_ser.sin_port = htons(m_root["UDPServer"]["Port"].asInt());
    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 250;
    setsockopt(m_socket_handler, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
    if(bind(m_socket_handler,(struct sockaddr*)&m_sock_ser, sizeof(m_sock_ser))==-1)
        LOG(FATAL)<<"Can not bind UDP server at "<<m_root["UDPServer"]["IP"].asString()<<":"<<m_root["UDPServer"]["Port"].asInt();
    else
        LOG(INFO)<<"Bind UDP server at "<<m_root["UDPServer"]["IP"].asString()<<":"<<m_root["UDPServer"]["Port"].asInt();

    LOG(INFO)<<"Construct GNSS simulator.";
}

bool GNSSSimulator::reset()
{
    m_final_score = 0;
    m_max_error_score_p = 0;
    m_max_error_score_v = 0;
    m_scores_p.clear();
    m_scores_v.clear();
    m_scores_p_vector.clear();
    m_scores_v_vector.clear();
    m_scores_p_gt_vector.clear();
    m_scores_v_gt_vector.clear();
    m_shutdown_flag = true;
    m_run_thread_ptr->join();
    while(!m_run_thread_exit)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    m_results_mutex.lock();
    m_status = Status::pending;
    m_results["map"] = 1;
    m_results["status"] = "pending";
    Json::Value monitor;
    monitor["status"] = "normal";
    monitor["enter"] = "";  
    monitor["ouput"] = "";
    m_results["monitor"] = monitor;
    Json::Value result_inner;
    result_inner["score"] = 0.0;
    result_inner["positionAccuracy"] = 0.0;
    result_inner["offset"] = 0.0;
    result_inner["speedAccuracy"] = 0.0;
    result_inner["availability"] = 0.0;
    result_inner["resultPath"] = "";
    result_inner["status"] = "";
    result_inner["resultMD5"] = "";
    result_inner["recordPath"] = "";
    result_inner["recordMD5"] = "";
    m_results["results"] = result_inner;
    m_results_mutex.unlock();

    while(1)
    {
        int ret = recvfrom(m_socket_handler, &m_curr_data_block_offset, 4, 0, 0, 0);
        if(ret==-1)break;
    }

    m_curr_data_idx = 0;
    m_curr_buffer_size = 0;
    m_curr_data_block_offset = 0;

    // start();
    return true;
}

GNSSSimulator::~GNSSSimulator()
{
    m_shutdown_flag = true;
    if(m_run_thread_ptr->joinable())m_run_thread_ptr->join();
    while(!m_run_thread_exit)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    delete[] m_buffer_ptr;
    delete[] m_tmp_buffer_ptr;
    LOG(INFO)<<"Destruct gnsssimulator";
}

void GNSSSimulator::start()
{
    m_run_thread_ptr = std::make_unique<std::thread>(
        [this]()
        {
            std::function<double(std::array<double, 7>, std::array<double, 7>)> evaluate_ep_one_point_func = 
                [](std::array<double, 7>gt, std::array<double, 7>caculated){
                    double ep = std::sqrt((gt[1]- caculated[1])* (gt[1]- caculated[1]) + 
                        (gt[2]- caculated[2])* (gt[2]- caculated[2]) + (gt[3]- caculated[3])* (gt[3]- caculated[3]));
                    return ep;
            };
            std::function<double(std::array<double, 7>, std::array<double, 7>)> evaluate_ev_one_point_func = 
                [](std::array<double, 7>gt, std::array<double, 7>caculated){
                    double ev = std::sqrt((gt[4]- caculated[4])* (gt[4]- caculated[4]) + 
                        (gt[5]- caculated[5])* (gt[5]- caculated[5]) + (gt[6]- caculated[6])* (gt[6]- caculated[6]));
                    return ev;
            };
            m_shutdown_flag = false;
            char udp_rec_buff[100];
            uint addr_len = sizeof(m_sock_client);
            m_second_count = 0;
            m_output_aligned = false;
            m_not_aligned_count = 0;
            m_gt_cal_after_aligned_count = 0;
            m_received_count = 0;
            std::fstream output_f;
            double last_valid_timestamp;
            output_f.open("output.txt", std::ios::out|std::ios::binary);
            if(!output_f.is_open())LOG(FATAL)<<"Failed to open output.txt.";
            LOG(INFO)<<"GT: "<<m_ground_true.size();
            LOG(INFO)<<"Create run thread.";
            m_run_thread_exit = false;
            while(!m_shutdown_flag)
            {
                int ret = recvfrom(m_socket_handler, udp_rec_buff, 100, 0, (struct sockaddr*)&m_sock_client, (uint*)&addr_len);
                if(ret==-1)continue;
                udp_rec_buff[ret] = 0;
                LOG(INFO)<<"receive message from udp client: "<<inet_ntoa(m_sock_client.sin_addr)<<":"<<htons(m_sock_client.sin_port)<<", message: "<<udp_rec_buff;
                if(strncmp(udp_rec_buff, m_root["StartSignal"]["Message"].asCString(), m_root["StartSignal"]["Length"].asInt())==0)break;
            }
            if(!m_shutdown_flag)
            {
                LOG(INFO)<<"Game start.";
                m_status = Status::running;
                m_results_mutex.lock();
                m_results["status"] = "running";
                m_results_mutex.unlock();
            }
            for(int i = 0; i < m_data_files_vec.size(); i++)
            {
                LOG(INFO)<<"Data file vector: "<<m_data_files_vec[i];
            }

            while(m_curr_data_idx < (m_data_num-2) && !m_shutdown_flag)
            {
                auto start = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                while(m_curr_buffer_size < m_sending_data_size_per_second && !m_shutdown_flag)
                {
                    if(m_curr_data_idx ==0 && m_curr_data_block_offset == 0)
                    {
                        std::ifstream inFile(m_data_files_vec[m_curr_data_idx],std::ios::in|std::ios::binary); //二进制读方式打开
                        if(!inFile) {
                            LOG(FATAL)<< "Can not read data file: "<<m_data_files_vec[m_curr_data_idx];
                        }
                        inFile.read(m_tmp_buffer_ptr, m_data_block_size);
                        // LOG(INFO)<<"Read "<<m_data_files_vec[m_curr_data_idx]<< " into tmp buffer.";
                        for(int i =0; i<m_data_block_size;i+=4)
                        {
                            //LOG(INFO)<<*(m_tmp_buffer_ptr+i)<<*(m_tmp_buffer_ptr+i+1)<<*(m_tmp_buffer_ptr+i+2)<<*(m_tmp_buffer_ptr+i+3)<<*(m_tmp_buffer_ptr+i+4);
                            if(*(m_tmp_buffer_ptr+i)==' ' && *(m_tmp_buffer_ptr+i+1)==' ' && *(m_tmp_buffer_ptr+i+2)==' ' && *(m_tmp_buffer_ptr+i+3)==' ' && *(m_tmp_buffer_ptr+i+4)!=' ')
                            {
                                // LOG(INFO)<<"For the first data block data start at "<<i+4  <<"th byte.";
                                m_curr_data_block_offset = i+4;
                                break;
                            }
                        }
                    }else
                    {
                        if(m_curr_data_block_offset == m_data_block_size)
                        {
                            m_curr_data_idx += 1;
                            std::ifstream inFile(m_data_files_vec[m_curr_data_idx],std::ios::in|std::ios::binary); //二进制读方式打开
                            if(!inFile) {
                                LOG(FATAL)<< "Can not read data file: "<<m_data_files_vec[m_curr_data_idx];
                            }
                            inFile.read(m_tmp_buffer_ptr, m_data_block_size);
                            // LOG(INFO)<<"Read "<<m_data_files_vec[m_curr_data_idx]<< " into tmp buffer.";
                            m_curr_data_block_offset = 0;
                        }
                        if((m_data_block_size - m_curr_data_block_offset) > m_sending_data_size_per_second - m_curr_buffer_size)
                        {
                            memcpy(m_buffer_ptr+m_curr_buffer_size, m_tmp_buffer_ptr+m_curr_data_block_offset, m_sending_data_size_per_second - m_curr_buffer_size);
                            // LOG(INFO)<<"Read "<<m_sending_data_size_per_second - m_curr_buffer_size<<" data from tmp buffer at "<<m_curr_data_block_offset<< " into buffer at "<< m_curr_buffer_size;
                            m_curr_data_block_offset += (m_sending_data_size_per_second - m_curr_buffer_size);
                            m_curr_buffer_size = m_sending_data_size_per_second; 
                            // LOG(INFO)<<"Current buffer size: "<<m_curr_buffer_size;
                        }else
                        {
                            memcpy(m_buffer_ptr+m_curr_buffer_size, m_tmp_buffer_ptr+m_curr_data_block_offset, m_data_block_size - m_curr_data_block_offset);
                            // LOG(INFO)<<"Read "<<m_data_block_size - m_curr_data_block_offset<<" data from tmp buffer at "<<m_curr_data_block_offset<< " into buffer at "<< m_curr_buffer_size;
                            m_curr_buffer_size +=  (m_data_block_size - m_curr_data_block_offset);
                            m_curr_data_block_offset = m_data_block_size;
                            // LOG(INFO)<<"Current buffer size: "<<m_curr_buffer_size;
                        }

                    }

                }
                int send_count = m_sending_data_size_per_second/m_packet_size;
                // LOG(INFO)<<"Time stamp: "<<m_start_timestamp;
                // if(sendto(m_socket_handler, &m_start_timestamp, 8, 0, (struct sockaddr*)&m_sock_client,sizeof(sockaddr))==-1)
                // {
                //     perror("");
                //     LOG(FATAL)<<"Send buffer failed.";
                // }
                for(int i = 0; i<send_count; i++)
                {
                    if(sendto(m_socket_handler, m_buffer_ptr+m_packet_size*i, m_packet_size, 0, (struct sockaddr*)&m_sock_client,sizeof(sockaddr))==-1)
                    {
                        perror("");
                        LOG(FATAL)<<"Send buffer failed.";
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    //LOG(INFO)<<"Send "<< m_packet_size <<" from "<<m_packet_size*i;
                }
                if(sendto(m_socket_handler, m_buffer_ptr+m_packet_size*send_count, m_sending_data_size_per_second%m_packet_size, 0, (struct sockaddr*)&m_sock_client,sizeof(sockaddr))==-1)
                {
                    perror("");
                    LOG(FATAL)<<"Send buffer failed.";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                //LOG(INFO)<<"Send "<< m_sending_data_size_per_second%m_packet_size <<" from "<<m_packet_size*send_count;
                LOG(INFO)<<"send buffer.";   
                int one_loop_rec_count = 0;
                while(!m_shutdown_flag)
                {
                    int ret = recvfrom(m_socket_handler, udp_rec_buff, 100, 0, (struct sockaddr*)&m_sock_client, (uint*)&addr_len);
                    bool valid = false;
                    // LOG(INFO)<<"ret: " << ret; 
                    // LOG(INFO)<<"Not aligned count: "<<m_not_aligned_count;
                    if(ret==sizeof(m_decodeddata))
                    {
                        memcpy(&m_decodeddata, udp_rec_buff, ret);
                        uint32_t crc = crc32((uint8_t*)&m_decodeddata, 57);
                        if(crc == m_decodeddata.crc && one_loop_rec_count < 5)
                        {
                            if(!m_output_aligned)
                            {
                                if(m_decodeddata.timestamp > m_ground_true[m_second_count*5][0] && m_decodeddata.timestamp < m_ground_true[(m_second_count+1)*5+1500][0])
                                {
                                    valid = true;
                                    m_output_aligned = true;
                                    // LOG(INFO)<<"Valid not aligned.";
                                    for(int i = m_second_count*5; i < (m_second_count+1)*5+1500; i++)
                                    {
                                        if(fabs(m_ground_true[i][0]-  m_decodeddata.timestamp)<0.001)
                                        {
                                            LOG(INFO)<<i<<" "<<m_ground_true[i][0] <<" "<<m_decodeddata.timestamp;
                                            m_aligned_first_gt_idx = i;
                                            LOG(INFO)<<"First aligned gt idx: "<<m_aligned_first_gt_idx;
                                            break;
                                        }
                                    }
                                    LOG(INFO)<<"First aligned: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp<<" "<<m_decodeddata.px<<" "<<m_decodeddata.py<<" "<<m_decodeddata.pz
                                        <<" "<<m_decodeddata.vx<<" "<<m_decodeddata.vy<<" "<<m_decodeddata.vz<<" "<<m_decodeddata.crc<<std::endl;
                                    LOG(INFO)<<"First aligngt: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                    m_caculate_path.push_back({m_decodeddata.timestamp, m_decodeddata.px, m_decodeddata.py, m_decodeddata.pz,
                                        m_decodeddata.vx, m_decodeddata.vy, m_decodeddata.vz});
                                    if(m_second_count>=m_root["InitFrames"].asInt())
                                    {
                                        double sp = evaluate_ep_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                        m_caculate_path.back());
                                        double sv = evaluate_ev_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                        m_caculate_path.back());
                                        LOG(INFO)<<"Score: "<<sp<<" "<<sv;
                                        m_scores_p.push_back(sp);
                                        m_scores_v.push_back(sv);
                                        m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                        m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                        m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                        m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                        if(m_max_error_score_p < sp)m_max_error_score_p = sp;
                                        if(m_max_error_score_v < sv)m_max_error_score_v = sv;
                                    }
                                    m_aligned_first_caculated_idx = (m_second_count-30) * 5 + one_loop_rec_count;
                                    last_valid_timestamp = m_decodeddata.timestamp;
                                    one_loop_rec_count += 1;
                                    m_gt_cal_after_aligned_count += 1;
                                }else
                                {
                                    one_loop_rec_count += 1;
                                    m_caculate_path.push_back({0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
                                    LOG(INFO)<<"Not Aligned: " << std::fixed << std::setprecision(5) << 0<<" "<<0<<" "<<0<<" "<<0
                                        <<" "<<0<<" "<<0<<" "<<std::endl;
                                    LOG(INFO)<<"Aligngt: NO"<<std::endl;
                                    if(m_second_count>=m_root["InitFrames"].asInt())
                                    {
                                        double s = 0;
                                        LOG(INFO)<<"Score: "<<s << s;
                                        m_scores_p.push_back(s);
                                        m_scores_v.push_back(s);
                                        // if(m_max_error_score_p < s)m_max_error_score_p = s;
                                        // if(m_max_error_score_v < s)m_max_error_score_v = s;
                                        m_scores_p_gt_vector.push_back({0, 
                                            0,
                                            0});
                                        m_scores_v_gt_vector.push_back({0, 
                                            0,
                                            0});
                                        m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                        m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                    }
                                    // LOG(INFO)<<"InValid caculated.";
                                }
                            }else
                            {
                                LOG(INFO)<<"last in output aligned: "<< std::fixed << std::setprecision(5)<< std::fixed << std::setprecision(5)<<last_valid_timestamp;
                                LOG(INFO)<<"current timestamp: "<< std::fixed << std::setprecision(5)<< std::fixed << std::setprecision(5)<<m_decodeddata.timestamp; 
                                LOG(INFO)<<"score size:" << m_scores_p.size()<<" "<<m_scores_v.size();
                                LOG(INFO)<<"socre max error: "<< m_max_error_score_p <<" "<<m_max_error_score_v;
                                if(fabs(m_decodeddata.timestamp -  last_valid_timestamp - 0.2)<0.0001)
                                {
                                    last_valid_timestamp = m_decodeddata.timestamp;
                                    valid=true;

                                    m_caculate_path.push_back({m_decodeddata.timestamp, m_decodeddata.px, m_decodeddata.py, m_decodeddata.pz,
                                        m_decodeddata.vx, m_decodeddata.vy, m_decodeddata.vz});
                                    LOG(INFO)<<"Aligned0.2: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp<<" "<<m_decodeddata.px<<" "<<m_decodeddata.py<<" "<<m_decodeddata.pz
                                    <<" "<<m_decodeddata.vx<<" "<<m_decodeddata.vy<<" "<<m_decodeddata.vz<<" "<<m_decodeddata.crc<<std::endl;
                                    LOG(INFO)<<"Aligngt0.2: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                    if(m_second_count>=m_root["InitFrames"].asInt())
                                    {
                                        double sp = evaluate_ep_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                        m_caculate_path.back());
                                        double sv = evaluate_ev_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                        m_caculate_path.back());
                                        LOG(INFO)<<"Score: "<<sp<<" "<<sv;
                                        m_scores_p.push_back(sp);
                                        m_scores_v.push_back(sv);
                                        m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                        m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                        m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                        m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                        if(m_max_error_score_p < sp)m_max_error_score_p = sp;
                                        if(m_max_error_score_v < sv)m_max_error_score_v = sv;
                                    }
                                    one_loop_rec_count += 1;
                                    m_gt_cal_after_aligned_count += 1;
                                }else if(fabs(m_decodeddata.timestamp -  last_valid_timestamp - 0.4)<0.0001)
                                {
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;

                                        m_caculate_path.push_back({m_decodeddata.timestamp-0.2, 0, 0, 0, 0, 0, 0});
                                        LOG(INFO)<<"Aligned0.4: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp-0.2<<" "<<0<<" "<<0<<" "<<0
                                            <<" "<<0<<" "<<0<<" "<<0<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt0.4: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double s = 0;
                                            LOG(INFO)<<"Score: "<<s<<" "<<s;
                                            m_scores_p.push_back(s);
                                            m_scores_v.push_back(s);
                                        m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                        m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                        m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                        m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                        // if(m_max_error_score_p < sp && m_caculate_path.back()[1]!=0)m_max_error_score_p = sp;
                                        // if(m_max_error_score_v < sv && m_caculate_path.back()[1]!=0)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;

                                        m_caculate_path.push_back({m_decodeddata.timestamp, m_decodeddata.px, m_decodeddata.py, m_decodeddata.pz,
                                        m_decodeddata.vx, m_decodeddata.vy, m_decodeddata.vz});
                                        LOG(INFO)<<"Aligned0.4: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp<<" "<<m_decodeddata.px<<" "<<m_decodeddata.py<<" "<<m_decodeddata.pz
                                        <<" "<<m_decodeddata.vx<<" "<<m_decodeddata.vy<<" "<<m_decodeddata.vz<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt0.4: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                        double sp = evaluate_ep_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                        m_caculate_path.back());
                                        double sv = evaluate_ev_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                        m_caculate_path.back());
                                        LOG(INFO)<<"Score: "<<sp<<" "<<sv;
                                        m_scores_p.push_back(sp);
                                        m_scores_v.push_back(sv);
                                        m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                        m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                        m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                        m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                        if(m_max_error_score_p < sp)m_max_error_score_p = sp;
                                        if(m_max_error_score_v < sv )m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                }else if(fabs(m_decodeddata.timestamp -  last_valid_timestamp - 0.6)<0.0001)
                                {
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp-0.4, 0, 0, 0, 0, 0, 0});
                                        LOG(INFO)<<"Aligned0.6: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp-0.4<<" "<<0<<" "<<0<<" "<<0
                                            <<" "<<0<<" "<<0<<" "<<0<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt0.6: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double s = 0;
                                            LOG(INFO)<<"Score: "<<s<<" "<<s;
                                            m_scores_p.push_back(s);
                                            m_scores_v.push_back(s);
                                        m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                        m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                            m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                        m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                        m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                        // if(m_max_error_score_p < sp && m_caculate_path.back()[1]!=0)m_max_error_score_p = sp;
                                        // if(m_max_error_score_v < sv && m_caculate_path.back()[1]!=0)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp-0.2, 0, 0, 0, 0, 0, 0});
                                        LOG(INFO)<<"Aligned0.6: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp-0.2<<" "<<0<<" "<<0<<" "<<0
                                            <<" "<<0<<" "<<0<<" "<<0<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt0.6: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double s = 0;
                                            LOG(INFO)<<"Score: "<<s<<" "<<s;
                                            m_scores_p.push_back(s);
                                            m_scores_v.push_back(s);
                                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                            // if(m_max_error_score_p < sp && m_caculate_path.back()[1]!=0)m_max_error_score_p = sp;
                                            // if(m_max_error_score_v < sv && m_caculate_path.back()[1]!=0)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp, m_decodeddata.px, m_decodeddata.py, m_decodeddata.pz,
                                        m_decodeddata.vx, m_decodeddata.vy, m_decodeddata.vz});
                                        LOG(INFO)<<"Aligned0.6: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp<<" "<<m_decodeddata.px<<" "<<m_decodeddata.py<<" "<<m_decodeddata.pz
                                            <<" "<<m_decodeddata.vx<<" "<<m_decodeddata.vy<<" "<<m_decodeddata.vz<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt0.6: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double sp = evaluate_ep_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                            m_caculate_path.back());
                                            double sv = evaluate_ev_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                            m_caculate_path.back());
                                            LOG(INFO)<<"Score: "<<sp<<" "<<sv;
                                            m_scores_p.push_back(sp);
                                            m_scores_v.push_back(sv);
                                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                            if(m_max_error_score_p < sp)m_max_error_score_p = sp;
                                            if(m_max_error_score_v < sv)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                }else if(fabs(m_decodeddata.timestamp -  last_valid_timestamp - 0.8)<0.0001)
                                {
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp-0.6, 0, 0, 0, 0, 0, 0});
                                        LOG(INFO)<<"Aligned0.8: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp-0.6<<" "<<0<<" "<<0<<" "<<0
                                            <<" "<<0<<" "<<0<<" "<<0<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt0.8: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double s = 0;
                                            LOG(INFO)<<"Score: "<<s<<" "<<s;
                                            m_scores_p.push_back(s);
                                            m_scores_v.push_back(s);
                                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                            // if(m_max_error_score_p < sp && m_caculate_path.back()[1]!=0)m_max_error_score_p = sp;
                                            // if(m_max_error_score_v < sv && m_caculate_path.back()[1]!=0)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp-0.4, 0, 0, 0, 0, 0, 0});
                                        LOG(INFO)<<"Aligned0.8: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp-0.4<<" "<<0<<" "<<0<<" "<<0
                                            <<" "<<0<<" "<<0<<" "<<0<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt0.8: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double s = 0;
                                            LOG(INFO)<<"Score: "<<s<<" "<<s;
                                            m_scores_p.push_back(s);
                                            m_scores_v.push_back(s);
                                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                            // if(m_max_error_score_p < sp && m_caculate_path.back()[1]!=0)m_max_error_score_p = sp;
                                            // if(m_max_error_score_v < sv && m_caculate_path.back()[1]!=0)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp-0.2, 0, 0, 0, 0, 0, 0});
                                        LOG(INFO)<<"Aligned0.8: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp-0.2<<" "<<0<<" "<<0<<" "<<0
                                            <<" "<<0<<" "<<0<<" "<<0<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt0.8: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double s = 0;
                                            LOG(INFO)<<"Score: "<<s<<" "<<s;
                                            m_scores_p.push_back(s);
                                            m_scores_v.push_back(s);
                                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                            // if(m_max_error_score_p < sp && m_caculate_path.back()[1]!=0)m_max_error_score_p = sp;
                                            // if(m_max_error_score_v < sv && m_caculate_path.back()[1]!=0)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp, m_decodeddata.px, m_decodeddata.py, m_decodeddata.pz,
                                        m_decodeddata.vx, m_decodeddata.vy, m_decodeddata.vz});
                                        LOG(INFO)<<"Aligned0.8: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp<<" "<<m_decodeddata.px<<" "<<m_decodeddata.py<<" "<<m_decodeddata.pz
                                            <<" "<<m_decodeddata.vx<<" "<<m_decodeddata.vy<<" "<<m_decodeddata.vz<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt0.8: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double sp = evaluate_ep_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                            m_caculate_path.back());
                                            double sv = evaluate_ev_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                            m_caculate_path.back());
                                            LOG(INFO)<<"Score: "<<sp<<" "<<sv;
                                            m_scores_p.push_back(sp);
                                            m_scores_v.push_back(sv);
                                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                            if(m_max_error_score_p < sp)m_max_error_score_p = sp;
                                            if(m_max_error_score_v < sv)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                }else if(fabs(m_decodeddata.timestamp -  last_valid_timestamp - 1.0)<0.0001)
                                {
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp-0.8, 0, 0, 0, 0, 0, 0});
                                        LOG(INFO)<<"Aligned1.0: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp-0.8<<" "<<0<<" "<<0<<" "<<0
                                            <<" "<<0<<" "<<0<<" "<<0<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt1.0: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double s = 0;
                                            LOG(INFO)<<"Score: "<<s<<" "<<s;
                                            m_scores_p.push_back(s);
                                            m_scores_v.push_back(s);
                                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                            // if(m_max_error_score_p < sp && m_caculate_path.back()[1]!=0)m_max_error_score_p = sp;
                                            // if(m_max_error_score_v < sv && m_caculate_path.back()[1]!=0)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp-0.6, 0, 0, 0, 0, 0, 0});
                                        LOG(INFO)<<"Aligned1.0: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp-0.6<<" "<<0<<" "<<0<<" "<<0
                                            <<" "<<0<<" "<<0<<" "<<0<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt1.0: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double s = 0;
                                            LOG(INFO)<<"Score: "<<s<<" "<<s;
                                            m_scores_p.push_back(s);
                                            m_scores_v.push_back(s);
                                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                            // if(m_max_error_score_p < sp && m_caculate_path.back()[1]!=0)m_max_error_score_p = sp;
                                            // if(m_max_error_score_v < sv && m_caculate_path.back()[1]!=0)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp-0.4, 0, 0, 0, 0, 0, 0});
                                        LOG(INFO)<<"Aligned1.0: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp-0.4<<" "<<0<<" "<<0<<" "<<0
                                            <<" "<<0<<" "<<0<<" "<<0<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt1.0: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double s = 0;
                                            LOG(INFO)<<"Score: "<<s<<" "<<s;
                                            m_scores_p.push_back(s);
                                            m_scores_v.push_back(s);
                                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                            // if(m_max_error_score_p < sp && m_caculate_path.back()[1]!=0)m_max_error_score_p = sp;
                                            // if(m_max_error_score_v < sv && m_caculate_path.back()[1]!=0)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp-0.2, 0, 0, 0, 0, 0, 0});
                                        LOG(INFO)<<"Aligned1.0: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp-0.2<<" "<<0<<" "<<0<<" "<<0
                                            <<" "<<0<<" "<<0<<" "<<0<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt1.0: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            // double sp = evaluate_ep_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                            // m_caculate_path.back());
                                            // double sv = evaluate_ev_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                            // m_caculate_path.back());

                                            double s = 0;
                                            LOG(INFO)<<"Score: "<<s<<" "<<s;
                                            m_scores_p.push_back(s);
                                            m_scores_v.push_back(s);
                                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                            // if(m_max_error_score_p < sp && m_caculate_path.back()[1]!=0)m_max_error_score_p = sp;
                                            // if(m_max_error_score_v < sv && m_caculate_path.back()[1]!=0)m_max_error_score_v = sv;  
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                    if(one_loop_rec_count < 5)
                                    {
                                        last_valid_timestamp += 0.2;
                                        m_caculate_path.push_back({m_decodeddata.timestamp, m_decodeddata.px, m_decodeddata.py, m_decodeddata.pz,
                                        m_decodeddata.vx, m_decodeddata.vy, m_decodeddata.vz});
                                        LOG(INFO)<<"Aligned1.0: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp<<" "<<m_decodeddata.px<<" "<<m_decodeddata.py<<" "<<m_decodeddata.pz
                                            <<" "<<m_decodeddata.vx<<" "<<m_decodeddata.vy<<" "<<m_decodeddata.vz<<" "<<m_decodeddata.crc<<std::endl;
                                        LOG(INFO)<<"Aligngt1.0: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                                        if(m_second_count>=m_root["InitFrames"].asInt())
                                        {
                                            double sp = evaluate_ep_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                            m_caculate_path.back());
                                            double sv = evaluate_ev_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                                            m_caculate_path.back());
                                            LOG(INFO)<<"Score: "<<sp<<" "<<sv;
                                            m_scores_p.push_back(sp);
                                            m_scores_v.push_back(sv);
                                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                                            if(m_max_error_score_p < sp)m_max_error_score_p = sp;
                                            if(m_max_error_score_v < sv)m_max_error_score_v = sv;
                                        }
                                        one_loop_rec_count += 1;
                                        m_gt_cal_after_aligned_count += 1;
                                    }
                                }
                            }
                            // LOG(INFO)<<"Receive: " << std::fixed << std::setprecision(5) << m_decodeddata.timestamp<<" "<<m_decodeddata.px<<" "<<m_decodeddata.py<<" "<<m_decodeddata.pz
                            // <<" "<<m_decodeddata.vx<<" "<<m_decodeddata.vy<<" "<<m_decodeddata.vz<<" "<<m_decodeddata.crc<<std::endl;
                        }else
                        {
                            // LOG(INFO)<<"Receive crc: "<<m_decodeddata.crc<<", caculate crc: "<<crc<<std::endl;
                        }
                    }
                    // if(valid && !m_output_aligned)
                    // {
                    //     one_loop_rec_count += 1;
                    //     m_output_aligned = true;
                    //     LOG(INFO)<<"Valid not aligned.";
                    //     m_caculate_path.push_back({m_decodeddata.timestamp, m_decodeddata.px, m_decodeddata.py, m_decodeddata.pz,
                    //         m_decodeddata.vx, m_decodeddata.vy, m_decodeddata.vz});
                    //     m_aligned_first_caculated_idx = (m_second_count-30) * 5 + one_loop_rec_count;
                    //     for(int i = m_second_count*5; i < (m_second_count+1)*5+1500; i++)
                    //     {
                    //         if(fabs(m_ground_true[0][0]-  m_decodeddata.timestamp)<0.001)m_aligned_first_caculated_idx = i;
                    //     }
                    //     last_valid_timestamp = m_decodeddata.timestamp;
                    // }
                    // else if(valid && m_output_aligned)
                    // {
                    //     one_loop_rec_count += 1;
                    //     LOG(INFO)<<"Valid and aligned.";
                    //     m_caculate_path.push_back({m_decodeddata.timestamp, m_decodeddata.px, m_decodeddata.py, m_decodeddata.pz,
                    //         m_decodeddata.vx, m_decodeddata.vy, m_decodeddata.vz});
                    // }else if(!valid && !m_output_aligned)
                    // {
                    //     // one_loop_rec_count += 1;
                    //     // LOG(INFO)<<"Not Valid and not aligned.";
                    //     // m_caculate_path.push_back({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
                    // }else if(!valid && m_output_aligned)
                    // {
                    //     // one_loop_rec_count += 1;
                    //     // LOG(INFO)<<"Not Valid but aligned.";
                    //     // m_caculate_path.push_back({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
                    // }
                    if(m_not_aligned_count >= m_root["NotValidFailFrames"].asInt())
                    {
                        // LOG(INFO)<<"120 frame not valid, failed!";
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    now = std::chrono::steady_clock::now();
                    if(std::chrono::duration_cast<std::chrono::milliseconds>(now-start).count()>=1/m_send_freq*1000)break;
                }
                for(int i = 0; i < 5-one_loop_rec_count; i ++)
                {
                    if(m_output_aligned)
                    {
                        m_caculate_path.push_back({last_valid_timestamp+0.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
                        LOG(INFO)<<"Aligned out: " << std::fixed << std::setprecision(5) << last_valid_timestamp+0.2<<" "<<0<<" "<<0<<" "<<0
                            <<" "<<0<<" "<<0<<" "<<std::endl;
                        LOG(INFO)<<"Aligngt out: " << std::fixed << std::setprecision(5) << m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][0]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]
                        <<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5]<<" "<<m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]<<std::endl;
                        if(m_second_count>=m_root["InitFrames"].asInt())
                        {
                            // double sp = evaluate_ep_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                            // m_caculate_path.back());
                            // double sv = evaluate_ev_one_point_func(m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count], 
                            // m_caculate_path.back());
                            double s = 0;
                            LOG(INFO)<<"Score: "<<s<<" "<<s;
                            m_scores_p.push_back(s);
                            m_scores_v.push_back(s);
                            m_scores_p_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][1], 
                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][2],
                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][3]});
                            m_scores_v_gt_vector.push_back({m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][4], 
                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][5],
                                m_ground_true[m_aligned_first_gt_idx+m_gt_cal_after_aligned_count][6]});
                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                            // if(m_max_error_score_p < sp && m_caculate_path.back()[1]!=0)m_max_error_score_p = sp;
                            // if(m_max_error_score_v < sv && m_caculate_path.back()[1]!=0)m_max_error_score_v = sv;
                        }
                        m_gt_cal_after_aligned_count += 1;
                        last_valid_timestamp+=0.2;
                    }else
                    {
                        m_caculate_path.push_back({0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
                        LOG(INFO)<<"Not Aligned: " << std::fixed << std::setprecision(5) << 0<<" "<<0<<" "<<0<<" "<<0
                            <<" "<<0<<" "<<0<<" "<<std::endl;
                        LOG(INFO)<<"Aligngt: NO"<<std::endl;
                        if(m_second_count>=m_root["InitFrames"].asInt())
                        {
                            double s = 0;
                            LOG(INFO)<<"Score: "<<s << s;
                            m_scores_p.push_back(s);
                            m_scores_v.push_back(s);
                            // if(m_max_error_score_p < s)m_max_error_score_p = s;
                            // if(m_max_error_score_v < s)m_max_error_score_v = s;
                            m_scores_p_gt_vector.push_back({0, 
                                0,
                                0});
                            m_scores_v_gt_vector.push_back({0, 
                                0,
                                0});
                            m_scores_p_vector.push_back({m_caculate_path.back()[1], m_caculate_path.back()[2],m_caculate_path.back()[3]});
                            m_scores_v_vector.push_back({m_caculate_path.back()[4],m_caculate_path.back()[5],m_caculate_path.back()[6]});
                        }
                    }
                 
                }
                if(!m_output_aligned)m_not_aligned_count += 1;
                m_second_count += 1;
                LOG(INFO)<<"Send 1s bag count: "<<m_second_count;
                // LOG(INFO)<<"Time esplase "<<std::chrono::duration_cast<std::chrono::milliseconds>(now-start).count()<<" ms";
                m_curr_buffer_size = 0;
                // m_start_timestamp += 1000;
                if(m_not_aligned_count >= m_root["NotValidFailFrames"].asInt())
                {
                    m_results["results"]["status"] = "fail";
                    LOG(INFO)<<m_root["NotValidFailFrames"].asInt()<<" frame not valid, failed!";
                    break;
                }
            }
            // LOG(INFO)<<m_caculate_path.size()<<std::endl;
            for(int i =0; i < m_caculate_path.size(); i++)
            {
                for(int j = 0; j < m_caculate_path[0].size(); j ++)
                {
                    output_f.write((char*)&(m_caculate_path[i][j]), 8);
                    // std::cout<<m_caculate_path[i][j] <<", ";
                }
                // std::cout<<"\n";
            }

            // m_scores_p.pop_back();

            double max_error_p_vec0 = m_max_error_score_p / 1.7320508;
            double max_error_v_vec0 = m_max_error_score_v / 1.7320508;

            int first_not_0_gt_idx;
            for(int i =0; i<m_scores_p.size();i++)
            {
                if(m_scores_p_gt_vector[i][0] != 0)
                {
                    first_not_0_gt_idx = i;
                    break;
                }
            }

            for(int i = first_not_0_gt_idx; i > 0; i--)
            {
                LOG(INFO)<<i;
                m_scores_p_gt_vector[i-1][0] = m_ground_true[m_aligned_first_gt_idx-(6-first_not_0_gt_idx)][0];
                m_scores_p_gt_vector[i-1][1] = m_ground_true[m_aligned_first_gt_idx-(6-first_not_0_gt_idx)][1];
                m_scores_p_gt_vector[i-1][2] = m_ground_true[m_aligned_first_gt_idx-(6-first_not_0_gt_idx)][2];
            }
            for(int i =0; i<m_scores_p.size();i++)
            std::cout<<", gt: "<<m_scores_p_gt_vector[i][0]<<","<<m_scores_p_gt_vector[i][1]<<","<<m_scores_p_gt_vector[i][2]<<std::endl;

            int available_num = 0;
            for(int i =0; i<m_scores_p.size();i++)
            {
                // LOG(INFO)<<"M_scores: "<<i<<": "<<m_scores_p[i];
                if(m_scores_p[i] == 0)
                {
                    m_scores_p_vector[i][0] = m_scores_p_gt_vector[i][0] + max_error_p_vec0;
                    m_scores_p_vector[i][1] = m_scores_p_gt_vector[i][1] + max_error_p_vec0;
                    m_scores_p_vector[i][2] = m_scores_p_gt_vector[i][2] + max_error_p_vec0;
                    m_scores_v_vector[i][0] = m_scores_v_gt_vector[i][0] + max_error_v_vec0;
                    m_scores_v_vector[i][1] = m_scores_v_gt_vector[i][1] + max_error_v_vec0;
                    m_scores_v_vector[i][2] = m_scores_v_gt_vector[i][2] + max_error_v_vec0;
                // std::cout<<"11111m score p vector: "<<m_scores_p_vector[i][0]<<","<<m_scores_p_gt_vector[i][1]<<","<<m_scores_p_gt_vector[i][2]
                    // <<", gt: "<<m_scores_p_gt_vector[i][0]<<","<<m_scores_p_gt_vector[i][1]<<","<<m_scores_p_gt_vector[i][2]<<std::endl;
                }else
                {
                    available_num+=1;
                }
            }


            LOG(INFO)<<"Total data recv: "<<m_scores_p.size();
            LOG(INFO)<<"Available data: "<<available_num;

            LOG(INFO)<<"max error p: "<<max_error_p_vec0;
            LOG(INFO)<<"max error v: "<<max_error_v_vec0;
            std::array<double, 3> mean_error_vector_p={0,0,0};
            for(int i = 0; i <  m_scores_p_vector.size(); i++)
            {
                mean_error_vector_p[0] += (m_scores_p_vector[i][0] - m_scores_p_gt_vector[i][0]);
                mean_error_vector_p[1] += (m_scores_p_vector[i][1] - m_scores_p_gt_vector[i][1]);
                mean_error_vector_p[2] += (m_scores_p_vector[i][2] - m_scores_p_gt_vector[i][2]);
                std::cout<<"m score p vector: "<< std::fixed << std::setprecision(5)<<m_scores_p_vector[i][0]<<","<<m_scores_p_gt_vector[i][1]<<","<<m_scores_p_gt_vector[i][2]
                    <<", gt: "<<m_scores_p_gt_vector[i][0]<<","<<m_scores_p_gt_vector[i][1]<<","<<m_scores_p_gt_vector[i][2]<<std::endl;
            }
            mean_error_vector_p[0] /= m_scores_p_vector.size();
            mean_error_vector_p[1] /= m_scores_p_vector.size();
            mean_error_vector_p[2] /= m_scores_p_vector.size();

            m_average_offset_p = std::sqrt(mean_error_vector_p[0]*mean_error_vector_p[0]+mean_error_vector_p[1]*mean_error_vector_p[1]+
                mean_error_vector_p[2]*mean_error_vector_p[2]);

            LOG(INFO)<<"Mean vector p: "<<mean_error_vector_p[0]<<","<<mean_error_vector_p[1]<<","<<mean_error_vector_p[2];

            double rmsep = 0;
            double rmsev = 0;
            for(int i = 0; i <  m_scores_p_vector.size(); i++)
            {   
                std::array<double, 3> tmp_e;
                tmp_e[0] = mean_error_vector_p[0] - (m_scores_p_vector[i][0] - m_scores_p_gt_vector[i][0]);
                tmp_e[1] = mean_error_vector_p[1] - (m_scores_p_vector[i][1] - m_scores_p_gt_vector[i][1]);
                tmp_e[2] = mean_error_vector_p[2] - (m_scores_p_vector[i][2] - m_scores_p_gt_vector[i][2]);
                double tmp_e2 = tmp_e[0]*tmp_e[0]+tmp_e[1]*tmp_e[1]+tmp_e[2]*tmp_e[2];
                rmsep += tmp_e2;

                std::array<double, 3> tmp_ev;
                tmp_ev[0] = (m_scores_v_vector[i][0] - m_scores_v_gt_vector[i][0]);
                tmp_ev[1] = (m_scores_v_vector[i][1] - m_scores_v_gt_vector[i][1]);
                tmp_ev[2] = (m_scores_v_vector[i][2] - m_scores_v_gt_vector[i][2]);
                double tmp_ev2 = tmp_ev[0]*tmp_ev[0]+tmp_ev[1]*tmp_ev[1]+tmp_ev[2]*tmp_ev[2];
                rmsev += tmp_ev2;
            }
            rmsep /= m_scores_p_vector.size();
            rmsep = std::sqrt(rmsep);
            rmsev /= m_scores_v_vector.size();
            rmsev = std::sqrt(rmsev);

            LOG(INFO)<<"Rmsep: "<<rmsep;
            LOG(INFO)<<"Rmsev: "<<rmsev;

            m_final_score = rmsep * rmsev;

            if(m_shutdown_flag)
            {
                m_shutdown_flag = false;
                LOG(INFO)<<"Shutdown work flow.";
            }
            else
            {
                LOG(INFO)<<"Finished work flow.";
                m_status = Status::finished;
            } 
            double avaper = ((float)available_num / (float)m_scores_p.size());
            if(avaper < 0.8)
            {
                m_results_mutex.lock(); 
                m_results["status"] = "finished"; 
                m_results["results"]["status"] = "fail";
                m_results["results"]["score"] = 0; 
                m_results["results"]["offset"] = 0; 
                m_results["results"]["positionAccuracy"] = 0; 
                m_results["results"]["speedAccuracy"] = 0; 
                m_results["results"]["availability"] = avaper; 
            }else{
                m_results_mutex.lock(); 
                m_results["status"] = "finished"; 
                m_results["results"]["status"] = "finished"; 
                m_results["results"]["score"] = m_final_score; 
                m_results["results"]["offset"] = m_average_offset_p; 
                m_results["results"]["positionAccuracy"] = rmsep; 
                m_results["results"]["speedAccuracy"] = rmsev; 
                m_results["results"]["availability"] = avaper; 
            }

            m_results_mutex.unlock(); 
            m_run_thread_exit = true; 
            output_f.close();
            LOG(INFO)<<"Results: "<<m_results.toStyledString();
            LOG(INFO)<<"Exit run thread.";
        }
    );
}

void GNSSSimulator::getResult(std::string& result)
{
    m_results_mutex.lock();
    result = m_results.toStyledString();
    m_results_mutex.unlock();
}