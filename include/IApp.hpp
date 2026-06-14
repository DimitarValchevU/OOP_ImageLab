#ifndef IAPP_H

#define IAPP_H

#include"Utility.hpp"
#include"Image.h"
#include"Filter.hpp"
#include"Filters.hpp"
#include"FilterPipeline.hpp"

class IApp
{
private:
protected:
	explicit IApp() = default;

	struct ImagePipiline
	{
		Image input;
		TFilterPipeline filters;
		Image output;
	};

	std::vector<ImagePipiline> m_pipelines;

	auto getPipeline(const std::string& name) -> std::optional<std::reference_wrapper<ImagePipiline>>
	{
		for (auto& pipeline : m_pipelines)
		{
			if (pipeline.input.getImageName() == name)
				return pipeline;
		}
		return std::nullopt;
	}
	auto getPipeline(const std::string& name) const -> std::optional<std::reference_wrapper<const ImagePipiline>>
	{
		for (const auto& pipeline : m_pipelines)
		{
			if (pipeline.input.getImageName() == name)
				return pipeline;
		}
		return std::nullopt;
	}

	auto processPipeline(ImagePipiline& pipeline, const std::filesystem::path& path = {}) -> std::expected<void, ErrorType>
	{
		auto currentImage = pipeline.input;
		auto result = pipeline.filters.apply(currentImage);
		if (!result)
			return std::unexpected(result.error());

		pipeline.output = std::move(currentImage);
		result = pipeline.output.saveNetpbm(path);
		if (!result)
			return std::unexpected(result.error());

		return {};
	}
public:
	virtual ~IApp() = default;

	template<typename Derived>
		requires std::derived_from<std::decay_t<Derived>, IApp>&&
		requires(Derived&& self) { self._run(); }
	auto run(this Derived&& self) -> std::expected<void, ErrorType>
	{
		auto result = self._run();
		if (!result)
			return std::unexpected(result.error());

		return result;
	}
};

#endif // !IAPP_H
