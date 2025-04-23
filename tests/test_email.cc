#include "Sylar/EmailServer/email.h"
#include "Sylar/EmailServer/smtp.h"

void test() {
    Sylar::EMail::ptr email = Sylar::EMail::Create(
            "user@163.com", "passwd",
            "hello world", "<B>hi xxx</B>hell world", {"564628276@qq.com"});
    Sylar::EMailEntity::ptr entity = Sylar::EMailEntity::CreateAttach("Sylar/Sylar.h");
    if(entity) {
        email->addEntity(entity);
    }

    entity = Sylar::EMailEntity::CreateAttach("Sylar/address.cc");
    if(entity) {
        email->addEntity(entity);
    }

    auto client = Sylar::SmtpClient::Create("smtp.163.com", 465, true);
    if(!client) {
        std::cout << "connect smtp.163.com:25 fail" << std::endl;
        return;
    }

    auto result = client->send(email, true);
    std::cout << "result=" << result->result << " msg=" << result->msg << std::endl;
    std::cout << client->getDebugInfo() << std::endl;
    //result = client->send(email, true);
    //std::cout << "result=" << result->result << " msg=" << result->msg << std::endl;
    //std::cout << client->getDebugInfo() << std::endl;
}

int main(int argc, char** argv) {
    Sylar::IOManager iom(1);
    iom.schedule(test);
    iom.stop();
    return 0;
}
