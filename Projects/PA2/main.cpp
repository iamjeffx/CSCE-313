#include <iostream>
using namespace std;

int main() {
    while(true) {
        cout << "Shelldon Cooper$ ";
        string line;
        getline(cin, line);
        if(line == string("Exit")) {
            break;
        }
    }
    return 0;
}
