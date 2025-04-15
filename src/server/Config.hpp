#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <jsoncpp/json/json.h>

namespace storage{
    class Config{
        private:
            int server_port_;
            std::string server_ip_;
            std::string download_prefix_;
            std::string deep_storage_dir_;
            std::string low_storage_dir_;
            std::string storage_info_;

            static Config* instance_;

            Config():server_port_(8081){}

        public:
            static Config* GetInstance(){
                static Config instance;
                return &instance;
            }

            bool Load(const std::string& config_file="./Storage.conf"){
                std::ifstream file(config_file);
                if(!file.is_open()){
                    std::cerr << "无法打开配置文件: " << config_file << std::endl;
                    return false;
                }

                Json::Value root;
                Json::CharReaderBuilder builder;
                std::string errs;

                if(!Json::parseFromStream(builder,file,&root,&errs)){
                    std::cerr << "解析配置文件失败: " << errs << std::endl;
                    return false;
                }

                server_port_ = root["server_port"].asInt();
                server_ip_ = root["server_ip"].asString();
                download_prefix_ = root["download_prefix"].asString();
                deep_storage_dir_ = root["deep_storage_dir"].asString();
                low_storage_dir_ = root["low_storage_dir"].asString();
                storage_info_ = root["storage_info"].asString();
                
                return true;
            }
            
            int GetServerPort() const { return server_port_; }
            std::string GetServerIp() const { return server_ip_; }
            std::string GetDownloadPrefix() const { return download_prefix_; }
            std::string GetDeepStorageDir() const { return deep_storage_dir_; }
            std::string GetLowStorageDir() const { return low_storage_dir_; }
            std::string GetStorageInfoFile() const { return storage_info_; }

    };
}