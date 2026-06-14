#ifndef FILTER_HPP

#define FILTER_HPP

#include"Utility.hpp"
#include"Image.h"

class Filter
{
private:
protected:
	explicit Filter() = default;

	template <typename RGBPixelFunction, typename GrayPixelFunction>
	auto processIndividualPixels(Image& image, RGBPixelFunction rgbPixelFunction, GrayPixelFunction grayPixelFunction) const -> std::expected<void, ErrorType>
	{
		auto isRGB = image.getNetpbmType() == NetpbmType::PPM;

		if (isRGB)
		{
			auto rgbData = image.getRGBData();
			if (!rgbData)
				return std::unexpected(rgbData.error());

			for (auto& pixel : rgbData.value())
				rgbPixelFunction(pixel);
			auto result = image.setRGBData(rgbData.value());
			if (!result)
				return std::unexpected(result.error());

		}
		else
		{
			auto grayData = image.getGrayData();
			if (!grayData)
				return std::unexpected(grayData.error());

			for (auto& pixel : grayData.value())
				grayPixelFunction(pixel);
			auto result = image.setGrayData(grayData.value());
			if (!result)
				return std::unexpected(result.error());
		}

		return {};
	}
	template <typename RGBPixelFunction, typename GrayPixelFunction>
	auto accessIndividualPixels(const Image& image, RGBPixelFunction rgbPixelFunction, GrayPixelFunction grayPixelFunction) const -> std::expected<void, ErrorType>
	{
		auto isRGB = image.getNetpbmType() == NetpbmType::PPM;

		if (isRGB)
		{
			auto rgbData = image.getRGBData();
			if (!rgbData)
				return std::unexpected(rgbData.error());

			for (auto& pixel : rgbData.value())
				rgbPixelFunction(pixel);
		}
		else
		{
			auto grayData = image.getGrayData();
			if (!grayData)
				return std::unexpected(grayData.error());

			for (auto& pixel : grayData.value())
				grayPixelFunction(pixel);
		}

		return {};
	}
public:
	virtual ~Filter() = default;

	template<typename Derived>
		requires std::derived_from<std::decay_t<Derived>, Filter>&&
		requires(Derived&& self, Image& image) { self._apply(image); }
	auto apply(this Derived&& self, Image& image) -> std::expected<void, ErrorType>
	{
		if (!image.isValid())
			return std::unexpected(ErrorType::InvalidImage);

		auto& data = image.m_data;
		if (data.empty())
			return std::unexpected(ErrorType::InvalidImage);

		auto result = self._apply(image);
		if (!result)
		{
			image.invalidate();
			return std::unexpected(result.error());
		}

		return result;
	}

	virtual auto info() const -> std::string = 0;
};

class KernelFilter : public Filter
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

	auto _apply(Image& image) const -> std::expected<void, ErrorType>
	{
		if (m_kernel.size() == 0 || m_kernel[0].empty())
			return std::unexpected(ErrorType::InvalidFilter);

		auto width = image.getWidth();
		auto height = image.getHeight();
		auto isRGB = image.getNetpbmType() == NetpbmType::PPM;

		auto kernelHeight = m_kernel.size();
		auto kernelWidth = m_kernel[0].size();
		auto radiusX = kernelWidth / 2;
		auto radiusY = kernelHeight / 2;

		if (isRGB)
		{
			auto result = image.getRGBData();
			if (!result)
				return std::unexpected(result.error());

			const auto& originalData = result.value();
			auto newData = originalData;

			for (auto y = radiusY; y < height - radiusY; ++y)
			{
				for (auto x = radiusX; x < width - radiusX; ++x)
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
				return std::unexpected(set.error());
		}
		else
		{
			auto result = image.getGrayData();
			if (!result)
				return std::unexpected(result.error());

			const auto& originalData = result.value();
			auto newData = originalData;

			for (auto y = radiusY; y < height - radiusY; ++y)
			{
				for (auto x = radiusX; x < width - radiusX; ++x)
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
				return std::unexpected(set.error());
		}

		return {};
	}

	virtual auto info() const -> std::string override
	{
		return "base-kernel";
	}
};

#endif // !FILTER_HPP