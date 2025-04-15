#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <string.h>
#include <iostream>

void DefaultHandler(struct evhttp_request *req,void *arg){
    struct evbuffer *evb = evbuffer_new();

    // 添加一些内容到缓冲区
    evbuffer_add_printf(evb, "<html><body><h1>Hello, World!</h1></body></html>");

    evhttp_add_header(evhttp_request_get_output_headers(req),"Content-Type","text/htmp");

    evhttp_send_reply(req,HTTP_OK,"OK",evb);

    evbuffer_free(evb);
}

int main(){
    struct event_base *base = event_base_new();
    if(!base){
        std::cerr << "无法创建event_base" <<std::endl;
        return 1;
    }

    struct evhttp *http =evhttp_new(base);
    if(!http){
        std::cerr << "无法创建evhttp" <<std::endl;
        return 1;
    }

    evhttp_set_gencb(http,DefaultHandler,NULL);

    if(evhttp_bind_socket(http,"0.0.0.0", 8081)!=0){
        std::cerr << "无法绑定到8081" <<std::endl;
        return 1;
    }

    std::cout <<"服务启动，监听8081中.." <<std::endl;

    event_base_dispatch(base);

    evhttp_free(http);
    event_base_free(base);

    return 0;
}

