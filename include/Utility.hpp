#ifndef UTILITY_HPP

#define UTILITY_HPP

#include<expected>
#include<string>
#include<vector>
#include<memory>
#include<filesystem>
#include<cstdint>
#include<sstream>
#include<fstream>
#include<iostream>
#include<future>
#include<optional>
#include<functional>
#include<concepts>
#include<print>
#include<cmath>
#include<algorithm>
#include<utility>
#include<variant>
#include<any>

typedef float float32_t;

enum class ErrorType
{
	UnknownError,

	// Load / Save
	InvalidFilePath,
	InvalidNetpbmFormat,

	// Image Manipulation
	InvalidImage,
	InvalidPixel,
	InvalidImageData,

	// Filter Pipeline
	InvalidFilter,

	// UI
	NoImagePipeline,
	DuplicateImagePipeline
};

inline auto trim(std::string str) -> std::string
{
	while (!str.empty() && (str.back() == '\n' || str.back() == '\r' ||
		std::isspace(static_cast<unsigned char>(str.back()))))
		str.pop_back();

	auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c) {
		return std::isspace(c);
		});

	return std::string(start, str.end());
}

#endif // !UTILITY_HPP

