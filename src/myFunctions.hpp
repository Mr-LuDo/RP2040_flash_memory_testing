#include <Arduino.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>


template<typename type>
void myPrint (type var) {
    Serial.print(var);
}

template<typename type>
void cout (type var) {
    Serial.print(var);
}

void newLine () {
    Serial.println();

    String str;
    str.reserve(100);

    str = "hi";
    Serial.println(str);
    Serial.println(str.length());
    str += " hi";
    Serial.println(str);
    Serial.println(str.length());
    
    // char* str = &std::to_string(x);
    // Serial.print(str);
}

// void ostreamToSerial (std::ostream os) {
    
//     // Serial.println();
    
//     std::cout << &os ;
//     int x = 5;
//     // String str = std::to_string(x);

//     // Serial.print(*(String*)&std::to_string(x));

//     // Serial.print(os);

// }

class myCout {
    private:
        String _str;
    public:
        myCout(/* args */) {
            _str.reserve(100);
        }
        ~myCout();
        
        template<typename type>
        void operator << (type& var) {
            _str += var;
        }

        void print () { Serial.println(_str); }
        void clear () { _str = ""; }
        
};

// template<typename type>
// type myCout::operator << (type var) {
//     // Serial.print(var);
//     return var;
// }