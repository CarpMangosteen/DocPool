#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <vector>
#include <filesystem>

namespace storage {
    namespace fs=std::filesystem;


    class FileUtil {
        private:
            std::string filename_;

        public:
            FileUtil(const std::string& filename) : filename_(filename) {}//构造函数

            size_t FileSize(){
                struct stat s;
                if(stat(filename_.c_str(),&s)==-1){
                    std::cerr << "获取文件大小失败: " << strerror(errno) << std::endl;
                    return 0;
                }
                return s.st_size;
            }

            time_t LastModifyTime() {
                struct stat s;
                if (stat(filename_.c_str(), &s) == -1) {
                    std::cerr << "获取文件修改时间失败: " << strerror(errno) << std::endl;
                    return 0;
                }
                return s.st_mtime;
            }
            
            time_t LastAccessTime() {
                struct stat s;
                if (stat(filename_.c_str(), &s) == -1) {
                    std::cerr << "获取文件访问时间失败: " << strerror(errno) << std::endl;
                    return 0;
                }
                return s.st_atime;
            }

            std::string FileName(){
                auto pos = filename_.find_last_of("/\\");
                if(pos==std::string::npos)
                    return filename_;
                return filename_.substr(pos+1);
            }

            bool GetContent(std::string* content){
                std::ifstream file(filename_,std::ios::binary);
                if(!file.is_open()){
                    std::cerr <<"无法打开文件："<<filename_ <<std::endl;
                    return false;
                }

                file.seekg(0,std::ios::end);
                content->resize(file.tellg());
                file.seekg(0,std::ios::beg);
                file.read(&(*content)[0],content->size());
                file.close();

                return true;
            }

            bool SetContent(const char* content, size_t len) {
                std::ofstream file(filename_, std::ios::binary);
                if (!file.is_open()) {
                    std::cerr << "无法打开文件进行写入: " << filename_ << std::endl;
                    return false;
                }
                
                file.write(content, len);
                file.close();
                
                return true;
            }

            bool Exists(){
                return fs::exists(filename_);
            }

            bool CreateDirectory(){
                if(Exists())
                    return true;
                return fs::create_directories(filename_);
            }

            bool ScanDirectory(std::vector<std::string>* files){//有改进空间
                if(!Exists()){
                    CreateDirectory();
                }

                for(const auto& entry : fs::directory_iterator(filename_)){
                    if(!fs::is_directory(entry.path())){
                        files->push_back(entry.path().string());
                    }
                }

                return true;
            }
    };
}