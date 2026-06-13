#ifndef FILTER_HPP

#define FILTER_HPP

#include<expected>
#include<string>
#include<vector>
#include<memory>
#include<cmath>
#include<algorithm>
#include<functional>
#include<utility>
#include<variant>
#include<any>
#include<concepts>

#include"Utility.hpp"
#include"Image.h"

class InversionFilter;
class ContrastNormalizationFilter;
class SharpenFilter;
class BlurFilter;
class SobelEdgeDetectionFilter;

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
};

template <typename... Filters>
class FilterPipeline
{
private:
	std::tuple<std::vector<Filters>...> m_filters;

	using FilterRef = std::variant<std::reference_wrapper<Filters>...>;
	std::vector<FilterRef> m_filterOrder;
protected:
public:

	template< typename Filter>
	auto addFilter(Filter&& filter) -> std::expected<void, ErrorType>
	{
		using FilterType = std::decay_t<Filter>;

		auto& filterVector = std::get<std::vector<FilterType>>(m_filters);
		filterVector.push_back(std::forward<Filter>(filter));
		m_filterOrder.push_back(std::ref(filterVector.back()));

		return {};
	}

	auto apply(Image& image) -> std::expected<void, ErrorType>
	{
		for (const auto& filterRef : m_filterOrder)
		{
			auto result = std::visit([&image](auto& filter) {
				return filter.get().apply(image);
				}, filterRef);

			if (!result)
				return std::unexpected(result.error());
		}
		return {};
	}

	auto listFilterNames() const -> std::vector<std::string>
	{
		auto filterNames = std::vector<std::string>{};
		for (const auto& filterRef : m_filterOrder)
		{
			std::visit([&filterNames](auto& filter) {
				using FilterType = std::decay_t<decltype(filter.get())>;
				if constexpr (std::is_same_v<FilterType, InversionFilter>)
					filterNames.push_back("InversionFilter");
				else if constexpr (std::is_same_v<FilterType, ContrastNormalizationFilter>)
					filterNames.push_back("ContrastNormalizationFilter");
				else if constexpr (std::is_same_v<FilterType, SharpenFilter>)
					filterNames.push_back("SharpenFilter");
				else if constexpr (std::is_same_v<FilterType, BlurFilter>)
					filterNames.push_back("BlurFilter");
				else if constexpr (std::is_same_v<FilterType, SobelEdgeDetectionFilter>)
					filterNames.push_back("SobelEdgeDetectionFilter");
				else
					filterNames.push_back("Unknown Filter Type");
				}, filterRef);
		}
		return filterNames;
	}

	auto removeFilter(size_t index) -> std::expected<std::any, ErrorType>
	{
		if (index >= m_filterOrder.size())
			return std::unexpected(ErrorType::InvalidFilter);

		auto removedFilter = std::visit([](auto& filter) -> std::any {
			return std::any(filter.get());
			}, m_filterOrder[index]);

		auto removedFilterRef = m_filterOrder[index];
		m_filterOrder.erase(m_filterOrder.begin() + index);

		std::apply([&](auto&... vectors)
			{
				auto removeFromVector = [&](auto& vector) {
					using VectorType = std::decay_t<decltype(vector)>;
					using FilterType = typename VectorType::value_type;
					if (std::holds_alternative<std::reference_wrapper<FilterType>>(removedFilterRef))
					{
						auto& filterToRemove = std::get<std::reference_wrapper<FilterType>>(removedFilterRef).get();
						vector.erase(std::remove_if(vector.begin(), vector.end(),
							[&](const FilterType& filter) { return &filter == &filterToRemove; }),
							vector.end());
					}
					};
				(removeFromVector(vectors), ...);
			}, m_filters);


		auto vectorIndexes = std::vector<size_t>(sizeof...(Filters), 0);
		for (auto& filterRef : m_filterOrder)
		{
			std::apply([&](auto&... vectors) {
				auto typeIndex = size_t{ 0 };
				(([&]() {
					using VectorType = std::decay_t<decltype(vectors)>;
					using FilterType = typename VectorType::value_type;

					if (std::holds_alternative<std::reference_wrapper<FilterType>>(filterRef))
					{
						filterRef = std::ref(vectors[vectorIndexes[typeIndex]++]);
					}
					++typeIndex;
					}()), ...);
				}, m_filters);
		}

		return removedFilter;
	}
};

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
};

class SharpenFilter : public KernelFilter
{
private:
protected:
public:
	explicit SharpenFilter(float32_t sharpness = 1.0f) : KernelFilter
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

class SobelEdgeDetectionFilter : public Filter
{
private:
	bool m_thresholdEnabled;
	uint8_t m_threshold;
protected:
public:
	explicit SobelEdgeDetectionFilter(bool thresholdEnabled = false, uint8_t threshold = 128) : m_thresholdEnabled(thresholdEnabled), m_threshold(threshold) {}

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
					if (m_thresholdEnabled)
					{
						pixel.r = (magnitudeR >= m_threshold) ? 255 : 0;
						pixel.g = (magnitudeG >= m_threshold) ? 255 : 0;
						pixel.b = (magnitudeB >= m_threshold) ? 255 : 0;
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
					if (m_thresholdEnabled)
					{
						pixel.g = (magnitudeG >= m_threshold) ? 255 : 0;
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
};

#endif // !FILTER_HPP

using TFilterPipeline = FilterPipeline
<
	InversionFilter,
	ContrastNormalizationFilter,
	SharpenFilter,
	BlurFilter,
	SobelEdgeDetectionFilter
>;