#include <thread>
#include <chrono>
#include "json/json.h"
#include "glog/logging.h"
#include <fstream>
#include <sys/stat.h>
#include <filesystem>
#include "gnss_simulator.h"

void initGlog()
{
    const char* dir = "/log";
    struct stat sb;
    if (stat(dir, &sb) != 0)std::filesystem::create_directories(dir);
    FLAGS_log_dir = dir;
    FLAGS_max_log_size=100;
    google::SetStderrLogging(google::GLOG_INFO);
    FLAGS_colorlogtostderr = true;
    google::InitGoogleLogging("");
    LOG(INFO)<<"Initial glog.";
}

int main(int argc, char** argv)
{
    initGlog();
    std::string configPath;
    if(argc ==2)configPath = argv[1];
    else configPath = "/config/config.json";
    std::fstream f(configPath, std::ios::in);
    if(!f.is_open())
    {
        LOG(FATAL)<<"Can not open config.json";
    }
    Json::Value config;
    f >> config;
    LOG(INFO)<<"Read config:"<<config.toStyledString();
    f.close();

// ==========================================================================

// =================simulator workflow=======================================
    Simulator* gnsssimulator = new GNSSSimulator(config.toStyledString());
    gnsssimulator->start();
// ===========================================================================

    while(1)
    {
        if(gnsssimulator->getStatus() == Status::running)
        {
            LOG_EVERY_N(INFO, 1000)<<"Simulator is running.";
        }else if(gnsssimulator->getStatus() == Status::pending)
        {
            LOG_EVERY_N(INFO, 1000)<<"Simulator is pending.";
        }else
        {
            LOG_EVERY_N(INFO, 1000)<<"Simulator is finished.";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    gnsssimulator->reset();
    Json::Value res;
    res["result"] = "success";
    res["info"] = "";
    LOG(INFO)<<"Closed simulator.";
    // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    delete gnsssimulator;

    return 0;
}