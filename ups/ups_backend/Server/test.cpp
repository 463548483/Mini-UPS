#include <ext/stdio_filebuf.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main()
{
    ofstream ofs("test.txt");
    ofs << "Writing to a basic_ofstream object..." << endl;
    ofs.close();

    int posix_handle = fileno(::fopen("test.txt", "r"));

    __gnu_cxx::stdio_filebuf<char> filebuf(posix_handle, std::ios::in); // 1
    istream is(&filebuf); // 2

    string line;
    getline(is, line);
    cout << "line: " << line << std::endl;
    return 0;
}
