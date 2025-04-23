#include "Sylar/uri.h"
#include <iostream>

int main(int argc, char** argv) {
    //Sylar::Uri::ptr uri = Sylar::Uri::Create("http://www.Sylar.top/test/uri?id=100&name=Sylar#frg");
    //Sylar::Uri::ptr uri = Sylar::Uri::Create("http://admin@www.Sylar.top/test/中文/uri?id=100&name=Sylar&vv=中文#frg中文");
    Sylar::Uri::ptr uri = Sylar::Uri::Create("http://admin@www.Sylar.top");
    //Sylar::Uri::ptr uri = Sylar::Uri::Create("http://www.Sylar.top/test/uri");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}
