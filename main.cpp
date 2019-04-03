#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <iostream>
#include <string>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>

using namespace std;

char getPin(int pin){
	if(pin < 0 || pin > 15){
		cerr << "Wrong Pin" << endl;
		return 0;
	}
	char res = 0xB0; //Set SGL since using single ended measure 
	res |= (pin >> 1); //Set A0-A2
	res |= ((pin % 2) << 3); //Set ODD/Sign bit using mod to detect if pin# is even or odd
	return res;
}

int main(int argc, char* argv[]){
	int fd = wiringPiI2CSetup(0x45);

	int pin = 0;
	if(argc > 1)
		pin = atoi(argv[1]);

	string url = "http://142.93.35.60/app/data/new";
	char byte1 = getPin(pin);
	char byte2 = 0x80;
	
	curlpp::Cleanup cleaner;
	curlpp::Easy request;

	request.setOpt(new curlpp::options::Url(url.c_str()));
	request.setOpt(new curlpp::options::Verbose(true));

	std::list<std::string> header;
	header.push_back("Content-Type: application/x-www-form-urlencoded");
	
	request.setOpt(new curlpp::options::HttpHeader(header));
	
	while(1){
		wiringPiI2CWrite(fd, (int)byte1);
		wiringPiI2CWrite(fd, (int)byte2);

		wiringPiI2CWrite(fd, 0x80);
		wiringPiI2CWrite(fd, 0x00);

		delay(150);

		char a = wiringPiI2CRead(fd);
		char b = wiringPiI2CRead(fd);
		char c = wiringPiI2CRead(fd);

		delay(150);

		signed int voltage = ((a & 0x3F) << 10 | (b << 2) | (c >> 6))/2;
		cout << "Voltage : " << voltage << endl;
		float resist = voltage/65536.0*5.0;
		resist = resist*2000.0/(5-resist);
		cout << resist << endl;
		string post = "id=0&value=" + to_string(resist);
		request.setOpt(new curlpp::options::PostFields(post));
		request.setOpt(new curlpp::options::PostFieldSize(post.length()));
		
		try{
			request.perform();

		}catch ( curlpp::LogicError & e ) {
			std::cout << e.what() << std::endl;
		}
		catch ( curlpp::RuntimeError & e ) {
			std::cout << e.what() << std::endl;
		}
		
		delay(10);
	}
	return 0;
}
