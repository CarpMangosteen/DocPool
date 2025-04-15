#include "Config.hpp"
#include "FileUtil.hpp"
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <iostream>
#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <mutex>

struct FileInfo{
    std::string filename;
    std::string path;
    std::string url;
    std::string storage_type;
    size_t size;
    time_t upload_time;
};

// 全局文件信息存储
std::vector<FileInfo> g_files;
std::mutex g_files_mutex;

std::string url_decode(const std::string& str){
    std::string result;
    char ch;
    int i,j;
    for(int i=0;i<str.length();++i){
        if(str[i]=='%'){
            sscanf(str.substr(i+1,2).c_str(),"%x",&j);
            ch=static_cast<char>(j);
            result+=ch;
            i=i+2;
        }else if(str[i]=='+'){
            result+=' ';
        }else{
            result+=str[i];
        }
    }
    return result;
}

void IndexHandler(struct evhttp_request *req, void *arg){
    //读取index.html
    storage::FileUtil file("./index.html");
    std::string content;

    if(!file.Exists() ||!file.GetContent(&content)){
        content = "<html><body><h1>文件存储服务</h1><p>欢迎使用文件存储服务！</p></body></html>";
    }

    struct evbuffer *evb=evbuffer_new();
    evbuffer_add(evb,content.c_str(),content.size());

    evhttp_add_header(evhttp_request_get_output_headers(req),"Content-Type","text/html;charset=UTF-8");
    evhttp_send_reply(req,HTTP_OK,"OK",evb);

    evbuffer_free(evb);
}

void UploadHandler(struct evhttp_request *req,void *arg){
    std::cout << "处理上传请求..." << std::endl;
    
    //获取请提u头
    struct evkeyvalq *headers=evhttp_request_get_input_headers(req);
    const char *filename_header=evhttp_find_header(headers,"FileName");
    const char *storage_type_header=evhttp_find_header(headers,"StorageType");

    if(!filename_header||!storage_type_header){
        std::cerr <<"缺少必要的请求头" <<std::endl;
        evhttp_send_reply(req,HTTP_BADREQUEST,"Bad Request",NULL);
        return;
    }

    //解码文件名
    std::string filename = url_decode(filename_header);
    std::string storage_type=storage_type_header;

    std::cout << "文件名: " << filename << std::endl;
    std::cout << "存储类型: " << storage_type << std::endl;

    //获取配置
    storage::Config *config = storage::Config::GetInstance();

    //确定存储路径
    std::string storage_dir;
    if (storage_type == "deep") {
        storage_dir = config->GetDeepStorageDir();
    } else {
        storage_dir = config->GetLowStorageDir();
    }

    // 确保存储目录存在
    storage::FileUtil dir(storage_dir);
    if (!dir.Exists()) {
        dir.CreateDirectory();
    }

    //构建完整文件路径
    std::string file_path=storage_dir+filename;

    //获取请求体中的文件内容
    struct evbuffer *input_buffer = evhttp_request_get_input_buffer(req);
    size_t len =evbuffer_get_length(input_buffer);

    if(len==0){
        std::cerr<<"请求体为空"<<std::endl;
        evhttp_send_reply(req,HTTP_BADREQUEST,"Bad Request",NULL);
        return;
    }

    //读取文件内容
    std::unique_ptr<char[]> data(new char[len]);
    evbuffer_copyout(input_buffer, data.get(), len);

    //保存文件
    storage::FileUtil file(file_path);
    bool success=file.SetContent(data.get(),len);

    if (!success) {
        std::cerr << "保存文件失败" << std::endl;
        evhttp_send_reply(req, HTTP_INTERNAL, "Internal Server Error", NULL);
        return;
    }

    //记录文件信息
    FileInfo info;
    info.filename=filename;
    info.path=file_path;
    info.url=config->GetDownloadPrefix()+filename;
    info.storage_type=storage_type;
    info.size=len;
    info.upload_time=time(NULL);

    {
        std::lock_guard<std::mutex> lock(g_files_mutex);
        g_files.push_back(info);
    }

    //返回成功响应
    struct evbuffer *evb =evbuffer_new();
    evbuffer_add_printf(evb, "{\"status\":\"success\",\"message\":\"文件上传成功\"}");
    evhttp_add_header(evhttp_request_get_output_headers(req),"Content-Type","application/json;charset=UTF-8");
    evhttp_send_reply(req,HTTP_OK,"OK",evb);

    evbuffer_free(evb);

    std::cout << "文件上传成功: " << file_path << std::endl;
}

void ListHandler(struct evhttp_request *req,void *arg){
    std::cout<<"处理文件列表请求..."<<std::endl;

    struct evbuffer *evb = evbuffer_new();

    //构建JSON响应
    std::string json="{\"files\":[";

    {
        std::lock_guard<std::mutex>lock(g_files_mutex);
        for(size_t i=0;i<g_files.size();++i){
            const FileInfo &info=g_files[i];

            json += "{";
            json += "\"name\":\"" + info.filename + "\",";
            json += "\"url\":\"" + info.url + "\",";
            json += "\"type\":\"" + std::string(info.storage_type == "deep" ? "深度存储" : "普通存储") + "\",";
            json += "\"size\":\"" + std::to_string(info.size) + "\"";
            json += "}";

            if(i<g_files.size()-1){
                json+=",";
            }
        }
    }

    json+="]}";

    evbuffer_add(evb,json.c_str(),json.size());

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json; charset=UTF-8");
    evhttp_send_reply(req, HTTP_OK, "OK", evb);
    
    evbuffer_free(evb);
}

void DownloadHandler(struct evhttp_request *req,void *arg){
    const char *uri=evhttp_request_get_uri(req);
    std::cout << "处理下载请求: " << uri << std::endl;

    //从uri中提取文件名
    std::string prefix=storage::Config::GetInstance()->GetDownloadPrefix();
    if(strncmp(uri,prefix.c_str(),prefix.size())!=0){
        evhttp_send_reply(req,HTTP_NOTFOUND,"Not Found",NULL);
        return;
    }

    std::string filename=uri+prefix.size();

    //查找文件信息
    FileInfo *info =nullptr;
    {
        std::lock_guard<std::mutex> lock(g_files_mutex);
        for(auto &f:g_files){
            if(f.filename==filename){
                info=&f;
                break;
            }
        }
    }

    if(!info){
        evhttp_send_reply(req,HTTP_NOTFOUND,"Not Found",NULL);
        return;
    }

    //读取文件内容
    storage::FileUtil file(info->path);
    std::string content;
    if(!file.GetContent(&content)){
        evhttp_send_reply(req,HTTP_INTERNAL,"Inrternal Server Error",NULL);
        return;
    }
    
    //发送文件内容
    struct evbuffer *evb=evbuffer_new();
    evbuffer_add(evb,content.c_str(),content.size());

    // 设置Content-Type和Content-Disposition头
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/octet-stream");
    std::string disposition = "attachment; filename=\"" + info->filename + "\"";
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Disposition", disposition.c_str());

    evhttp_send_reply(req, HTTP_OK, "OK", evb);
    
    evbuffer_free(evb);
    
    std::cout << "文件下载成功: " << info->path << std::endl;
}

void GenericHandler(struct evhttp_request *req,void *arg){
    const char *uri=evhttp_request_get_uri(req);
    std::cout << "收到请求: " << uri << std::endl;

    enum evhttp_cmd_type method = evhttp_request_get_command(req);

    // 根据URI路由到不同的处理函数
    if(method == EVHTTP_REQ_GET){
        if(strcmp(uri,"/")==0||strcmp(uri,"/index.html")==0){
            IndexHandler(req,arg);
        }else if(strcmp(uri,"list")==0){
            ListHandler(req,arg);
        }else if(strncmp(uri,storage::Config::GetInstance()->GetDownloadPrefix().c_str(),storage::Config::GetInstance()->GetDownloadPrefix().size())==0){
            DownloadHandler(req,arg);
        }else{
            //404
            struct evbuffer *evb = evbuffer_new();
            evbuffer_add_printf(evb, "<html><body><h1>404 Not Found</h1></body></html>");

            evhttp_add_header(evhttp_request_get_output_headers(req),"Content-Type", "text/html; charset=UTF-8");
            evhttp_send_reply(req,HTTP_OK,"OK",evb);

            evbuffer_free(evb);
        }
    }else if(method==EVHTTP_REQ_POST){
        if(strcmp(uri,"/upload")==0){
            UploadHandler(req,arg);
        }else{
            // 404 Not Found
            evhttp_send_reply(req, HTTP_NOTFOUND, "Not Found", NULL);
        }
    }else{
        // 方法不支持
        evhttp_send_reply(req, HTTP_BADMETHOD, "Method Not Allowed", NULL);
    }
}

int main(){

    storage::Config *config=storage::Config::GetInstance();
    if(!config->Load()){
        std::cerr << "加载配置失败，使用默认配置" << std::endl;
    }

    //创建存储目录
    storage::FileUtil(config->GetDeepStorageDir()).CreateDirectory();
    storage::FileUtil(config->GetLowStorageDir()).CreateDirectory();

    //创建eventbase
    struct event_base *base = event_base_new();
    if(!base){
        std::cerr << "无法创建event_base" << std::endl;
        return 1;
    }

    // 创建evhttp对象
    struct evhttp *http=evhttp_new(base);
    if (!http) {
        std::cerr << "无法创建evhttp" << std::endl;
        return 1;
    }

    // 设置通用回调
    evhttp_set_gencb(http, GenericHandler, NULL);

    //绑定到配置端口
    int port =config->GetServerPort();
    if(evhttp_bind_socket(http,"0.0.0.0",port)!=0){
        std::cerr << "无法绑定到端口" << port << std::endl;
        return 1;
    }

    std::cout << "服务器已启动，监听端口" << port << "..." << std::endl;
    std::cout << "- 深度存储目录: " << config->GetDeepStorageDir() << std::endl;
    std::cout << "- 普通存储目录: " << config->GetLowStorageDir() << std::endl;

    //开始事件循环
    event_base_dispatch(base);

    //清理资源
    evhttp_free(http);
    event_base_free(base);

    return 0;
}