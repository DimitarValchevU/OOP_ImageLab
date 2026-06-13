#include<sstream>
#include<fstream>
#include<iostream>

#include"Image.h"

auto Image::loadNetpbm(const std::filesystem::path& path) -> std::expected<void, ErrorType>
{
	try
	{
		auto ifs = std::ifstream{ path, std::ios::binary };
		if (!ifs)
			return std::unexpected(ErrorType::InvalidFilePath);

		m_path = path;
		m_name = path.stem().string();

		std::string netpbmFormat;
		ifs >> netpbmFormat;

		if (netpbmFormat == "P1" || netpbmFormat == "P4")
			m_netpbmType = NetpbmType::PBM;
		else if (netpbmFormat == "P2" || netpbmFormat == "P5")
			m_netpbmType = NetpbmType::PGM;
		else if (netpbmFormat == "P3" || netpbmFormat == "P6")
			m_netpbmType = NetpbmType::PPM;
		else
			return std::unexpected(ErrorType::InvalidFilePath);

		ifs >> m_width >> m_height;
		if (m_netpbmType != NetpbmType::PBM)
			ifs >> m_maxColorValue;

		ifs.ignore();

		auto totalPixels = size_t{ m_width * m_height };
		if (m_netpbmType == NetpbmType::PPM)
			m_data.resize(totalPixels * 3);
		else
			m_data.resize(totalPixels);

		if (netpbmFormat == "P1" || netpbmFormat == "P2" || netpbmFormat == "P3")
		{
			auto pixelValue = int32_t{};
			for (auto i = size_t{ 0 }; i < m_data.size(); ++i)
			{
				ifs >> pixelValue;
				m_data[i] = static_cast<std::uint8_t>(pixelValue);
			}
		}
		else
		{
			ifs.read(reinterpret_cast<char*>(m_data.data()), m_data.size());
		}

		m_isValid = true;
	}
	catch (const std::exception&)
	{
		return std::unexpected(ErrorType::UnknownError);
	}
	catch (...)
	{
		return std::unexpected(ErrorType::UnknownError);
	}

	return {};
}

auto Image::saveNetpbm(const std::filesystem::path& path) const->std::expected<void, ErrorType>
{
	try
	{
		if (!m_isValid)
			return std::unexpected(ErrorType::InvalidImage);

		auto usedPath = path.empty() ? m_path : path;
		auto ofs = std::ofstream{ usedPath, std::ios::binary };
		if (!ofs)
			return std::unexpected(ErrorType::InvalidFilePath);

		if (usedPath.extension() != ".pbm"
			&& usedPath.extension() != ".pgm"
			&& usedPath.extension() != ".ppm")
			return std::unexpected(ErrorType::InvalidFilePath);

		switch (m_netpbmType)
		{
		case NetpbmType::PBM:
			if (usedPath.extension() != ".pbm")
				return std::unexpected(ErrorType::InvalidNetpbmFormat);
			ofs << "P1\n";
			break;
		case NetpbmType::PGM:
			if (usedPath.extension() != ".pgm")
				return std::unexpected(ErrorType::InvalidNetpbmFormat);
			ofs << "P2\n";
			break;
		case NetpbmType::PPM:
			if (usedPath.extension() != ".ppm")
				return std::unexpected(ErrorType::InvalidNetpbmFormat);
			ofs << "P3\n";
			break;
		default:
			break;
		}

		ofs << m_width << " " << m_height << "\n";
		if (m_netpbmType != NetpbmType::PBM)
			ofs << m_maxColorValue << "\n";

		for (const auto& pixel : m_data)
			ofs << static_cast<int32_t>(pixel) << " ";
	}
	catch (const std::exception&)
	{
		return std::unexpected(ErrorType::UnknownError);
	}
	catch (...)
	{
		return std::unexpected(ErrorType::UnknownError);
	}

	return {};
}

auto Image::getImageName() const->std::string
{
	return m_name;
}

auto Image::getWidth() const->size_t
{
	return m_width;
}

auto Image::getHeight() const->size_t
{
	return m_height;
}

auto Image::getNetpbmType() const->NetpbmType
{
	return m_netpbmType;
}

auto Image::isValid() const -> bool
{
	return m_isValid;
}

auto Image::getRGBPixel(size_t x, size_t y) const->std::expected<RGBPixel, ErrorType>
{
	if (!m_isValid)
		return std::unexpected(ErrorType::InvalidImage);
	if (m_netpbmType != NetpbmType::PPM)
		return std::unexpected(ErrorType::InvalidNetpbmFormat);
	if (x >= m_width || y >= m_height)
		return std::unexpected(ErrorType::InvalidPixel);

	auto index = size_t{ (y * m_width + x) * 3 };
	auto pixel = RGBPixel{ m_data[index], m_data[index + 1], m_data[index + 2] };
	return pixel;
}

auto Image::setRGBPixel(size_t x, size_t y, const RGBPixel& pixel)->std::expected<void, ErrorType>
{
	if (!m_isValid)
		return std::unexpected(ErrorType::InvalidImage);
	if (m_netpbmType != NetpbmType::PPM)
		return std::unexpected(ErrorType::InvalidNetpbmFormat);
	if (x >= m_width || y >= m_height)
		return std::unexpected(ErrorType::InvalidPixel);

	auto index = size_t{ (y * m_width + x) * 3 };
	m_data[index] = pixel.r;
	m_data[index + 1] = pixel.g;
	m_data[index + 2] = pixel.b;
	return {};
}

auto Image::getGrayPixel(size_t x, size_t y) const->std::expected<GrayPixel, ErrorType>
{
	if (!m_isValid)
		return std::unexpected(ErrorType::InvalidImage);
	if (m_netpbmType == NetpbmType::PPM)
		return std::unexpected(ErrorType::InvalidNetpbmFormat);
	if (x >= m_width || y >= m_height)
		return std::unexpected(ErrorType::InvalidPixel);

	auto index = size_t{ y * m_width + x };
	auto pixel = GrayPixel{ m_data[index] };
	return pixel;
}

auto Image::setGrayPixel(size_t x, size_t y, const GrayPixel& pixel)->std::expected<void, ErrorType>
{
	if (!m_isValid)
		return std::unexpected(ErrorType::InvalidImage);
	if (m_netpbmType == NetpbmType::PPM)
		return std::unexpected(ErrorType::InvalidNetpbmFormat);
	if (x >= m_width || y >= m_height)
		return std::unexpected(ErrorType::InvalidPixel);

	auto index = size_t{ y * m_width + x };
	m_data[index] = pixel.g;
	return {};
}

auto Image::getRGBData() const -> const std::expected<std::vector<RGBPixel>, ErrorType>
{
	if (!m_isValid)
		return std::unexpected(ErrorType::InvalidImage);
	if (m_netpbmType != NetpbmType::PPM)
		return std::unexpected(ErrorType::InvalidNetpbmFormat);

	auto totalPixels = size_t{ m_width * m_height };
	auto rgbData = std::vector<RGBPixel>{};
	rgbData.reserve(totalPixels);

	for (auto i = size_t{ 0 }; i < totalPixels; ++i)
	{
		auto index = i * 3;
		rgbData.push_back(RGBPixel{ m_data[index], m_data[index + 1], m_data[index + 2] });
	}

	return rgbData;
}

auto Image::setRGBData(const std::vector<RGBPixel>& data) -> std::expected<void, ErrorType>
{
	if (!m_isValid)
		return std::unexpected(ErrorType::InvalidImage);
	if (m_netpbmType != NetpbmType::PPM)
		return std::unexpected(ErrorType::InvalidNetpbmFormat);
	if (data.size() != m_width * m_height)
		return std::unexpected(ErrorType::InvalidImageData);

	m_data.clear();
	m_data.reserve(data.size() * 3);

	for (const auto& pixel : data)
	{
		m_data.push_back(pixel.r);
		m_data.push_back(pixel.g);
		m_data.push_back(pixel.b);
	}

	return {};
}

auto Image::getGrayData() const -> const std::expected<std::vector<GrayPixel>, ErrorType>
{
	if (!m_isValid)
		return std::unexpected(ErrorType::InvalidImage);
	if (m_netpbmType == NetpbmType::PPM)
		return std::unexpected(ErrorType::InvalidNetpbmFormat);

	auto totalPixels = size_t{ m_width * m_height };
	auto grayData = std::vector<GrayPixel>{};
	grayData.reserve(totalPixels);

	for (auto i = size_t{ 0 }; i < totalPixels; ++i)
	{
		auto index = i;
		grayData.push_back(GrayPixel{ m_data[index] });
	}

	return grayData;
}

auto Image::setGrayData(const std::vector<GrayPixel>& data) -> std::expected<void, ErrorType>
{
	if (!m_isValid)
		return std::unexpected(ErrorType::InvalidImage);
	if (m_netpbmType == NetpbmType::PPM)
		return std::unexpected(ErrorType::InvalidNetpbmFormat);
	if (data.size() != m_width * m_height)
		return std::unexpected(ErrorType::InvalidImageData);

	m_data.clear();
	m_data.reserve(data.size());

	for (const auto& pixel : data)
	{
		m_data.push_back(pixel.g);
	}

	return {};
}