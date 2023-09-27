#pragma once

#include "simulator.h"
#include <filesystem>
#include <fstream>
#include <set>
#include "glog/logging.h"
#include <thread>
#include <chrono>
#include "json/json.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "crc.h"
#include <mutex>
#include <sstream>
#include <iomanip>


class GNSSSimulator:public Simulator
{
public:
    GNSSSimulator(const std::string& config_string);
    ~GNSSSimulator() override;
    bool reset() override;
    void start() override;
    void getResult(std::string& result) override;
private:
    std::vector<std::string> m_data_files_vec;
    Json::Value m_root, m_results;
    int m_packet_size, m_data_num, m_curr_data_block_offset, m_curr_data_idx, m_data_block_size, m_sending_data_size_per_second, m_curr_buffer_size;
    float m_send_freq;
    char* m_buffer_ptr, *m_tmp_buffer_ptr;
    std::unique_ptr<std::thread> m_run_thread_ptr;
    bool m_shutdown_flag, m_run_thread_exit;
    int m_socket_handler;
    struct sockaddr_in m_sock_ser, m_sock_client;
    std::mutex m_results_mutex;
    bool m_save_data_per_second;
    bool m_output_aligned;
    int64_t m_start_timestamp;
    std::vector<std::array<double, 7>> m_ground_true, m_caculate_path;
    double m_max_error_score_p, m_max_error_score_v, m_average_offset_p, m_final_score;
    std::vector<double> m_scores_p, m_scores_v;
    std::vector<std::array<double, 3>> m_scores_p_vector, m_scores_v_vector, m_scores_p_gt_vector, m_scores_v_gt_vector;
    int m_second_count, m_not_aligned_count, m_received_count, m_gt_cal_after_aligned_count;
    int m_aligned_first_caculated_idx, m_aligned_first_gt_idx;
    struct DecodedData
    {
        char header;
        double timestamp;
        double px;
        double py;
        double pz;
        double vx;
        double vy;
        double vz; 
        uint32_t crc;
    }m_decodeddata;
};
