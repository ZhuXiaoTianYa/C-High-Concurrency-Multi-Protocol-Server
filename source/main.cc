#include "server.hpp"

int main()
{
    Buffer buf;

    for (int i = 0; i < 300; i++)
    {
        std::string str = "hahahaha" + std::to_string(i) + "\n";
        buf.WriteStringAndMove(str);
    }
    while (buf.ReadAbleSize() > 0)
    {
        std::string tmp = buf.GetLineAndMove();
        std::cout << tmp;
    }

    // std::cout << buf.ReadAsStringAndMove(buf.ReadAbleSize());

    // buf.WriteStringAndMove(str);

    // Buffer buf1;
    // buf1.WriteBufferAndMove(buf);

    // std::string tmp = buf.ReadAsStringAndMove(buf.ReadAbleSize());
    // std::cout << tmp << std::endl;
    // std::cout << buf.ReadAbleSize() << std::endl;
    // std::cout << buf1.ReadAbleSize() << std::endl;

    return 0;
}