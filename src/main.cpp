#include<iostream>

#include"Image.h"

int main()
{
	auto image = Image{};
	std::cout << "ImageLab! : " << image.isValid() << std::endl;
	return 0;
}