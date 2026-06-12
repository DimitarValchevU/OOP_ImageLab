#ifndef FILTER_HPP

#define FILTER_HPP

#include<expected>
#include<string>
#include<vector>
#include<memory>
#include<cmath>
#include<algorithm>
#include<functional>

#include"Utility.hpp"
#include"Image.h"

template<typename Derived>
class Filter
{
private:
protected:
	typedef std::function<void(RGBPixel&)> RGBPixelFunction;
	typedef std::function<void(GrayPixel&)> GrayPixelFunction;
	auto processIndividualPixels(Image& image, RGBPixelFunction rgbPixelFunction, GrayPixelFunction grayPixelFunction) const -> std::expected<void, ErrorType>
	{
		auto width = image.getWidth();
		auto height = image.getHeight();
		auto isRGB = image.getNetpbmType() == NetpbmType::PPM;

		for (auto y = size_t{ 0 }; y < height; ++y)
		{
			for (auto x = size_t{ 0 }; x < width; ++x)
			{
				if (isRGB)
				{
					auto pixelResult = image.getRGBPixel(x, y);
					if (!pixelResult)
						return std::unexpected(pixelResult.error());

					auto pixel = pixelResult.value();
					rgbPixelFunction(pixel);

					auto setResult = image.setRGBPixel(x, y, pixel);
					if (!setResult)
						return std::unexpected(setResult.error());
				}
				else
				{
					auto pixelResult = image.getGrayPixel(x, y);
					if (!pixelResult)
						return std::unexpected(pixelResult.error());

					auto pixel = pixelResult.value();
					grayPixelFunction(pixel);

					auto setResult = image.setGrayPixel(x, y, pixel);
					if (!setResult)
						return std::unexpected(setResult.error());
				}
			}
		}
	}
public:
	auto apply(Image& image) const -> std::expected<void, ErrorType>
	{
		if (!image.isValid())
			return std::unexpected(ErrorType::InvalidImage);

		auto& data = image.m_data;
		if (data.empty())
			return std::unexpected(ErrorType::InvalidImage);

		auto result = static_cast<const Derived*>(this)->apply(image);
		if (!result)
			return std::unexpected(result.error());

		return result;
	}
};

class InversionFilter : public Filter<InversionFilter>
{
private:
protected:
public:
	auto apply(Image& image) const -> std::expected<void, ErrorType>
	{
		return processIndividualPixels(image,
			[](RGBPixel& pixel) -> RGBPixel
			{
				pixel.r = 255 - pixel.r;
				pixel.g = 255 - pixel.g;
				pixel.b = 255 - pixel.b;
			},
			[](GrayPixel& pixel) -> GrayPixel
			{
				pixel.g = 255 - pixel.g;
			});
	}
};

class ContrastNormalizationFilter : public Filter<ContrastNormalizationFilter>
{
private:
protected:
public:
	auto apply(Image& image) const -> std::expected<void, ErrorType>
	{
		auto width = image.getWidth();
		auto height = image.getHeight();
		auto isRGB = image.getNetpbmType() == NetpbmType::PPM;

		auto minValue = uint8_t{ 255 };
		auto maxValue = uint8_t{ 0 };

		auto findMinMax = processIndividualPixels(image,
			[&](RGBPixel& pixel) -> RGBPixel
			{
				minValue = std::min({ minValue, pixel.r, pixel.g, pixel.b });
				maxValue = std::max({ maxValue, pixel.r, pixel.g, pixel.b });
			},
			[&](GrayPixel& pixel) -> GrayPixel
			{
				minValue = std::min(minValue, pixel.g);
				maxValue = std::max(maxValue, pixel.g);
			});

		if (!findMinMax)
			return findMinMax;

		if (minValue == maxValue)
			return {};

		auto range = static_cast<uint8_t>(maxValue - minValue);

		return processIndividualPixels(image,
			[=](RGBPixel& pixel) -> RGBPixel
			{
				pixel.r = static_cast<uint8_t>(((pixel.r - minValue) * 255) / range);
				pixel.g = static_cast<uint8_t>(((pixel.g - minValue) * 255) / range);
				pixel.b = static_cast<uint8_t>(((pixel.b - minValue) * 255) / range);
			},
			[=](GrayPixel& pixel) -> GrayPixel
			{
				pixel.g = static_cast<uint8_t>(((pixel.g - minValue) * 255) / range);
			});
	}
};

class KernelFilter : public Filter<KernelFilter>
{
private:
	std::vector<std::vector<float32_t>> m_kernel;
protected:
	auto setKernel(const std::vector<std::vector<float32_t>>& kernel) -> void
	{
		m_kernel = kernel;
	}
public:
	explicit KernelFilter(const std::vector<std::vector<float32_t>>& kernel) : m_kernel(kernel) {}
	auto apply(Image& image) const -> std::expected<void, ErrorType>
	{
		if (m_kernel.empty() || m_kernel[0].empty())
			return std::unexpected(ErrorType::InvalidFilter);

		auto width = image.getWidth();
		auto height = image.getHeight();
		auto isRGB = image.getNetpbmType() == NetpbmType::PPM;

		auto kernelHeight = m_kernel.size();
		auto kernelWidth = m_kernel[0].size();
		auto radiusX = kernelWidth / 2;
		auto radiusY = kernelHeight / 2;

		auto tempRGBData = std::vector<RGBPixel>{};
		auto tempGrayData = std::vector<GrayPixel>{};

		if (isRGB)
		{
			auto result = image.getRGBData();
			if (!result)
				return std::unexpected(result.error());

			const auto& originalData = result.value();
			auto newData = originalData;

			for (auto y = size_t{ 0 }; y < height - radiusY; ++y)
			{
				for (auto x = size_t{ 0 }; x < width - radiusX; ++x)
				{
					auto sumR = float32_t{ 0.0f };
					auto sumG = float32_t{ 0.0f };
					auto sumB = float32_t{ 0.0f };

					for (auto ky = size_t{ 0 }; ky < kernelHeight; ++ky)
					{
						for (auto kx = size_t{ 0 }; kx < kernelWidth; ++kx)
						{
							auto pixelX = x + kx - radiusX;
							auto pixelY = y + ky - radiusY;
							if (pixelX < width && pixelY < height)
							{
								const auto& neighbourPixel = originalData[pixelY * width + pixelX];

								auto kernelValue = m_kernel[ky][kx];
								sumR += neighbourPixel.r * kernelValue;
								sumG += neighbourPixel.g * kernelValue;
								sumB += neighbourPixel.b * kernelValue;
							}
						}
					}

					auto& pixel = newData[y * width + x];
					pixel.r = static_cast<uint8_t>(std::clamp(sumR, 0.0f, 255.0f));
					pixel.g = static_cast<uint8_t>(std::clamp(sumG, 0.0f, 255.0f));
					pixel.b = static_cast<uint8_t>(std::clamp(sumB, 0.0f, 255.0f));
				}
			}
			auto set = image.setRGBData(newData);
			if (!set)
				return set;
		}
		else
		{
			auto result = image.getGrayData();
			if (!result)
				return std::unexpected(result.error());

			const auto& originalData = result.value();
			auto newData = originalData;

			for (auto y = size_t{ 0 }; y < height - radiusY; ++y)
			{
				for (auto x = size_t{ 0 }; x < width - radiusX; ++x)
				{
					auto sum = float32_t{ 0.0f };

					for (auto ky = size_t{ 0 }; ky < kernelHeight; ++ky)
					{
						for (auto kx = size_t{ 0 }; kx < kernelWidth; ++kx)
						{
							auto pixelX = x + kx - radiusX;
							auto pixelY = y + ky - radiusY;
							if (pixelX < width && pixelY < height)
							{
								const auto& neighbourPixel = originalData[pixelY * width + pixelX];

								auto kernelValue = m_kernel[ky][kx];
								sum += neighbourPixel.g * kernelValue;
							}
						}
					}

					auto& pixel = newData[y * width + x];
					pixel.g = static_cast<uint8_t>(std::clamp(sum, 0.0f, 255.0f));
				}
			}
			auto set = image.setGrayData(newData);
			if (!set)
				return set;
		}

		return {};
	}
};

class SharpeningFilter : public KernelFilter
{
private:
protected:
public:
	explicit SharpeningFilter(float32_t sharpness = 1.0f) : KernelFilter
	(
		{}
	) {
		if (sharpness <= 0.0f)
			sharpness = 1.0f;
		auto centerValue = float32_t{ 1.0f + 4.0f * sharpness };
		auto edgeValue = float32_t{ -sharpness };

		setKernel({
			{ 0.0f, edgeValue, 0.0f },
			{ edgeValue, centerValue, edgeValue },
			{ 0.0f, edgeValue, 0.0f },
			});
	}
};

class BlurFilter : public KernelFilter
{
private:
protected:
public:
	explicit BlurFilter(size_t kernelSize = 3) : KernelFilter
	(
		{}
	) {
		if (kernelSize < 3)
			kernelSize = 3;
		if (kernelSize % 2 == 0)
			++kernelSize;

		auto totalElements = static_cast<float32_t>(kernelSize * kernelSize);
		auto value = 1.0f / totalElements;

		setKernel(std::vector<std::vector<float32_t>>{kernelSize, std::vector<float32_t>(kernelSize, value)});
	}
};

#endif // !FILTER_HPP
