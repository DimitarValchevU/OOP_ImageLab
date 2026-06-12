#ifndef IMAGE_H

#define IMAGE_H

#include<expected>
#include<string>
#include<vector>
#include<memory>
#include<filesystem>
#include<cstdint>

#include"Utility.hpp"

enum class NetpbmType
{
	PBM,
	PGM,
	PPM
};

struct RGBPixel
{
	std::uint8_t r;
	std::uint8_t g;
	std::uint8_t b;
};

struct GrayPixel
{
	std::uint8_t g;
};

class Image
{
private:
	std::string m_name;
	std::filesystem::path m_path;
	size_t m_width{ 0 };
	size_t m_height{ 0 };
	size_t m_maxColorValue{ 0 };
	NetpbmType m_netpbmType{ 0 };

	bool m_isValid{ false };
	std::vector<uint8_t> m_data;

	template <typename Derived>
	friend class Filter;
	friend class InversionFilter;
	friend class ContrastNormalizationFilter;
	friend class KernelFilter;

protected:
public:
	explicit Image() = default;
	auto loadNetpbm(const std::filesystem::path& path) -> std::expected<void, ErrorType>;
	auto saveNetpbm(const std::filesystem::path& path) const -> std::expected<void, ErrorType>;

	auto getImageName() const -> std::string;
	auto getWidth() const -> size_t;
	auto getHeight() const -> size_t;
	auto getNetpbmType() const -> NetpbmType;

	auto isValid() const -> bool;

	auto getRGBPixel(size_t x, size_t y) const -> std::expected<RGBPixel, ErrorType>;
	auto setRGBPixel(size_t x, size_t y, const RGBPixel& pixel) -> std::expected<void, ErrorType>;
	auto getGrayPixel(size_t x, size_t y) const -> std::expected<GrayPixel, ErrorType>;
	auto setGrayPixel(size_t x, size_t y, const GrayPixel& pixel) -> std::expected<void, ErrorType>;

	auto getRGBData() const -> const std::expected<std::vector<RGBPixel>, ErrorType>;
	auto setRGBData(const std::vector<RGBPixel>& data) -> std::expected<void, ErrorType>;
	auto getGrayData() const -> const std::expected<std::vector<GrayPixel>, ErrorType>;
	auto setGrayData(const std::vector<GrayPixel>& data) -> std::expected<void, ErrorType>;
};


#endif // !IMAGE_H
