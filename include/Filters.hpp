#ifndef FILTERS_HPP

#define FILTERS_HPP

#include"Utility.hpp"
#include"Image.h"
#include"Filter.hpp"

class InversionFilter : public Filter
{
private:
protected:
public:
	auto _apply(Image& image) const -> std::expected<void, ErrorType>
	{
		return processIndividualPixels(image,
			[](RGBPixel& pixel) -> void
			{
				pixel.r = 255 - pixel.r;
				pixel.g = 255 - pixel.g;
				pixel.b = 255 - pixel.b;
			},
			[](GrayPixel& pixel) -> void
			{
				pixel.g = 255 - pixel.g;
			});
	}

	auto info() const -> std::string override
	{
		return "inversion";
	}
};

class ContrastNormalizationFilter : public Filter
{
private:
protected:
public:
	auto _apply(Image& image) const -> std::expected<void, ErrorType>
	{
		auto width = image.getWidth();
		auto height = image.getHeight();
		auto isRGB = image.getNetpbmType() == NetpbmType::PPM;

		auto minValue = uint8_t{ 255 };
		auto maxValue = uint8_t{ 0 };

		auto findMinMax = accessIndividualPixels(image,
			[&](const RGBPixel& pixel) -> void
			{
				minValue = std::min({ minValue, pixel.r, pixel.g, pixel.b });
				maxValue = std::max({ maxValue, pixel.r, pixel.g, pixel.b });
			},
			[&](const GrayPixel& pixel) -> void
			{
				minValue = std::min(minValue, pixel.g);
				maxValue = std::max(maxValue, pixel.g);
			});

		if (!findMinMax)
			return std::unexpected(findMinMax.error());

		if (minValue == maxValue)
			return {};

		auto range = static_cast<uint8_t>(maxValue - minValue);

		return processIndividualPixels(image,
			[=](RGBPixel& pixel) -> void
			{
				pixel.r = static_cast<uint8_t>(((pixel.r - minValue) * 255) / range);
				pixel.g = static_cast<uint8_t>(((pixel.g - minValue) * 255) / range);
				pixel.b = static_cast<uint8_t>(((pixel.b - minValue) * 255) / range);
			},
			[=](GrayPixel& pixel) -> void
			{
				pixel.g = static_cast<uint8_t>(((pixel.g - minValue) * 255) / range);
			});
	}

	auto info() const -> std::string override
	{
		return "contrast-normalization";
	}
};

struct SharpenFilterArgs
{
	float32_t sharpness = 1.0f;
};
class SharpenFilter : public KernelFilter
{
private:
	SharpenFilterArgs m_args;
protected:
public:
	explicit SharpenFilter(const SharpenFilterArgs& args) : KernelFilter
	(
		{}
	) {
		m_args = args;
		auto& sharpness = m_args.sharpness;

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

	auto info() const -> std::string override
	{
		return "sharpen" + std::format("[{:.2f}]", m_args.sharpness);
	}
};

struct BlurFilterArgs
{
	size_t kernelSize = 3;
};
class BlurFilter : public KernelFilter
{
private:
	BlurFilterArgs m_args;
protected:
public:
	explicit BlurFilter(const BlurFilterArgs& args) : KernelFilter
	(
		{}
	) {
		m_args = args;
		auto& kernelSize = m_args.kernelSize;

		if (kernelSize < 3)
			kernelSize = 3;
		if (kernelSize % 2 == 0)
			++kernelSize;

		auto totalElements = static_cast<float32_t>(kernelSize * kernelSize);
		auto value = 1.0f / totalElements;

		setKernel(std::vector<std::vector<float32_t>>{kernelSize, std::vector<float32_t>(kernelSize, value)});
	}

	auto info() const -> std::string override
	{
		return "blur" + std::format("[{}]", m_args.kernelSize);
	}
};

class GrayscaleFilter : public Filter
{
private:
protected:
public:
	explicit GrayscaleFilter() = default;

	auto _apply(Image& image) const -> std::expected<void, ErrorType>
	{
		return processIndividualPixels(image,
			[](RGBPixel& pixel) -> void {
				auto gray = uint8_t{
					static_cast<uint8_t>
					(
						0.299f * pixel.r +
						0.587f * pixel.g +
						0.114f * pixel.b
					)
				};

				pixel.r = gray;
				pixel.g = gray;
				pixel.b = gray;
			},
			[](GrayPixel&) -> void {

			});
	}

	auto info() const -> std::string override
	{
		return "grayscale";
	}
};

struct SobelFilterArgs
{
	bool thresholdEnabled = false;
	uint8_t threshold = 128;
};
class SobelEdgeDetectionFilter : public Filter
{
private:
	SobelFilterArgs m_args;
protected:
public:
	explicit SobelEdgeDetectionFilter(const SobelFilterArgs& args) : m_args(args) {}

	auto _apply(Image& image) const -> std::expected<void, ErrorType>
	{
		auto width = image.getWidth();
		auto height = image.getHeight();
		auto isRGB = image.getNetpbmType() == NetpbmType::PPM;

		auto dxKernel = std::vector<std::vector<float32_t>>
		{
			{ -1.0f, 0.0f, 1.0f },
			{ -2.0f, 0.0f, 2.0f },
			{ -1.0f, 0.0f, 1.0f }
		};
		auto dyKernel = std::vector<std::vector<float32_t>>
		{
			{ -1.0f, -2.0f, -1.0f },
			{ 0.0f, 0.0f, 0.0f },
			{ 1.0f, 2.0f, 1.0f }
		};

		if (isRGB)
		{
			auto result = image.getRGBData();
			if (!result)
				return std::unexpected(result.error());

			{
				auto copy = image;
				auto gs = GrayscaleFilter{};
				if (gs.apply(copy))
				{
					auto gResult = copy.getRGBData();
					if (gResult)
						result = gResult;
				}
			}

			const auto& originalData = result.value();
			auto newData = std::vector<RGBPixel>(originalData.size(), RGBPixel{ 0, 0, 0 });

			for (auto y = size_t{ 1 }; y < height - 1; ++y)
			{
				for (auto x = size_t{ 1 }; x < width - 1; ++x)
				{
					auto sumRx = float32_t{ 0.0f };
					auto sumGx = float32_t{ 0.0f };
					auto sumBx = float32_t{ 0.0f };

					auto sumRy = float32_t{ 0.0f };
					auto sumGy = float32_t{ 0.0f };
					auto sumBy = float32_t{ 0.0f };

					for (auto ky = size_t{ 0 }; ky < 3; ++ky)
					{
						for (auto kx = size_t{ 0 }; kx < 3; ++kx)
						{
							auto pixelX = x + kx - 1;
							auto pixelY = y + ky - 1;

							const auto& neighbourPixel = originalData[pixelY * width + pixelX];
							auto kernelValueX = dxKernel[ky][kx];
							sumRx += neighbourPixel.r * kernelValueX;
							sumGx += neighbourPixel.g * kernelValueX;
							sumBx += neighbourPixel.b * kernelValueX;
							auto kernelValueY = dyKernel[ky][kx];
							sumRy += neighbourPixel.r * kernelValueY;
							sumGy += neighbourPixel.g * kernelValueY;
							sumBy += neighbourPixel.b * kernelValueY;
						}
					}

					auto magnitudeR = static_cast<uint8_t>(std::clamp(std::sqrt(sumRx * sumRx + sumRy * sumRy), 0.0f, 255.0f));
					auto magnitudeG = static_cast<uint8_t>(std::clamp(std::sqrt(sumGx * sumGx + sumGy * sumGy), 0.0f, 255.0f));
					auto magnitudeB = static_cast<uint8_t>(std::clamp(std::sqrt(sumBx * sumBx + sumBy * sumBy), 0.0f, 255.0f));

					auto& pixel = newData[y * width + x];
					if (m_args.thresholdEnabled)
					{
						pixel.r = (magnitudeR >= m_args.threshold) ? 255 : 0;
						pixel.g = (magnitudeG >= m_args.threshold) ? 255 : 0;
						pixel.b = (magnitudeB >= m_args.threshold) ? 255 : 0;
					}
					else
					{
						pixel.r = magnitudeR;
						pixel.g = magnitudeG;
						pixel.b = magnitudeB;
					}
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
			auto newData = std::vector<GrayPixel>(originalData.size(), GrayPixel{ 0 });

			for (auto y = size_t{ 1 }; y < height - 1; ++y)
			{
				for (auto x = size_t{ 1 }; x < width - 1; ++x)
				{
					auto sumGx = float32_t{ 0.0f };
					auto sumGy = float32_t{ 0.0f };

					for (auto ky = size_t{ 0 }; ky < 3; ++ky)
					{
						for (auto kx = size_t{ 0 }; kx < 3; ++kx)
						{
							auto pixelX = x + kx - 1;
							auto pixelY = y + ky - 1;

							const auto& neighbourPixel = originalData[pixelY * width + pixelX];
							auto kernelValueX = dxKernel[ky][kx];
							sumGx += neighbourPixel.g * kernelValueX;
							auto kernelValueY = dyKernel[ky][kx];
							sumGy += neighbourPixel.g * kernelValueY;
						}
					}

					auto magnitudeG = static_cast<uint8_t>(std::clamp(std::sqrt(sumGx * sumGx + sumGy * sumGy), 0.0f, 255.0f));

					auto& pixel = newData[y * width + x];
					if (m_args.thresholdEnabled)
					{
						pixel.g = (magnitudeG >= m_args.threshold) ? 255 : 0;
					}
					else
					{
						pixel.g = magnitudeG;
					}
				}
			}
			auto set = image.setGrayData(newData);
			if (!set)
				return std::unexpected(set.error());
		}

		return {};
	}

	auto info() const -> std::string override
	{
		return "sobel" + std::format("[{}, {}]", m_args.thresholdEnabled, m_args.threshold);
	}
};

using FilterArgs = std::variant<
	std::monostate,
	SharpenFilterArgs,
	BlurFilterArgs,
	SobelFilterArgs
>;

#endif // !FILTERS_HPP