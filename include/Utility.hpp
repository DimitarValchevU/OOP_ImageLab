#ifndef UTILITY_HPP

#define UTILITY_HPP

typedef float float32_t;

enum class ErrorType
{
	UnknownError,

	InvalidFilePath,
	InvalidNetpbmFormat,

	InvalidImage,
	InvalidPixel,
	InvalidImageData,

	InvalidFilter
};

#endif // !UTILITY_HPP

