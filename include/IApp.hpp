#ifndef IAPP_H

#define IAPP_H

#include<expected>
#include<string>
#include<vector>
#include<memory>
#include<filesystem>
#include<sstream>
#include<cstdint>
#include<future>
#include<optional>
#include<functional>
#include<concepts>

#include"Utility.hpp"
#include"Image.h"
#include"Filter.hpp"

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
		pipeline.output.saveNetpbm(path);
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

#include<print>
#include<iostream>

class ConsoleApp : public IApp
{
private:

	auto loadImage(const std::string& path) -> std::expected<std::string, std::string>
	{
		auto message = std::string{};

		auto image = Image{};
		auto loadResult = image.loadNetpbm(path);
		if (!loadResult)
		{
			message = "Failed to load image: " + path + " - " + std::to_string(static_cast<int>(loadResult.error()));
			std::println("{}", message);
			return std::unexpected(message);
		}

		auto name = image.getImageName();
		if (getPipeline(name))
		{
			message = "Image with name '" + name + "' already exists in the pipeline.";
			std::println("{}", message);
			return std::unexpected(message);
		}

		m_pipelines.push_back(ImagePipiline{ std::move(image), {}, Image{} });
		message = "Image loaded successfully: " + path;
		std::println("{}", message);
		return message;

	}
	auto addFilter(const std::string& pipelineName, const std::string& filterType, float32_t param) -> std::expected<std::string, std::string>
	{
		auto message = std::string{};

		auto pipelineOptional = getPipeline(pipelineName);
		if (!pipelineOptional)
		{
			message = "No pipeline found for image name: " + pipelineName;
			std::println("{}", message);
			return std::unexpected(message);
		}

		auto& pipeline = pipelineOptional->get(); //TODO Perfect forwarding...
		if (filterType == "inversion")
			pipeline.filters.addFilter(InversionFilter{});
		else if (filterType == "contrast-normalization")
			pipeline.filters.addFilter(ContrastNormalizationFilter{});
		else if (filterType == "sharpen")
			pipeline.filters.addFilter(SharpenFilter{ param });
		else if (filterType == "blur")
			pipeline.filters.addFilter(BlurFilter{ static_cast<size_t>(param) });
		else if (filterType == "sobel")
			pipeline.filters.addFilter(SobelEdgeDetectionFilter{ false, static_cast<uint8_t>(param) });
		else
		{
			message = "Unknown filter type: " + filterType;
			std::println("{}", message);
			return std::unexpected(message);
		}

		message = "Filter added successfully to image: " + pipelineName;
		std::println("{}", message);
		return message;
	}
	auto removeFilter(const std::string& pipelineName, size_t filterIndex) -> std::expected<std::string, std::string>
	{
		auto message = std::string{};

		auto pipelineOptional = getPipeline(pipelineName);
		if (!pipelineOptional)
		{
			message = "No pipeline found for image name: " + pipelineName;
			std::println("{}", message);
			return std::unexpected(message);
		}

		auto& pipeline = pipelineOptional->get();
		auto removeResult = pipeline.filters.removeFilter(filterIndex);
		if (!removeResult)
		{
			message = "Failed to remove filter at index " + std::to_string(filterIndex) + " from image: " + pipelineName + " - " + std::to_string(static_cast<int>(removeResult.error()));
			std::println("{}", message);
			return std::unexpected(message);
		}

		message = "Filter removed successfully from image: " + pipelineName;
		std::println("{}", message);
		return message;
	}
	auto listPipeline(const std::string& pipelineName) const -> std::expected<std::string, std::string>
	{
		auto message = std::string{};

		auto pipelineOptional = getPipeline(pipelineName);
		if (!pipelineOptional)
		{
			message = "No pipeline found for image name: " + pipelineName;
			std::println("{}", message);
			return std::unexpected(message);
		}

		auto& pipeline = pipelineOptional->get();
		auto filterList = pipeline.filters.listFilterNames();

		if (filterList.empty())
		{
			message = "No filters found in pipeline for image: " + pipelineName;
			std::println("{}", message);
			return std::unexpected(message);
		}

		message = "Filters in pipeline for image " + pipelineName + ": \n";
		for (const auto& filterName : filterList)
			message += filterName + "\n";
		std::println("{}", message);
		return message;
	}
	auto listAllPipelines() const -> std::expected<std::string, std::string>
	{
		auto message = std::string{};
		if (m_pipelines.empty())
		{
			message = "No pipelines found.";
			std::println("{}", message);
			return std::unexpected(message);
		}

		message = {};
		for (const auto& pipeline : m_pipelines)
		{
			auto pipelineList = listPipeline(pipeline.input.getImageName());
			if (!pipelineList)
			{
				message = pipelineList.error();
				std::println("{}", message);
				return std::unexpected(message);
			}

			message += pipelineList.value();
		}
		std::println("{}", message);
		return message;
	}

	auto runPipeline(const std::string& pipelineName, const std::filesystem::path& path = {}) -> std::expected<std::string, std::string>
	{
		auto message = std::string{};
		std::println("Starting pipeline...");

		auto pipelineOptional = getPipeline(pipelineName);
		if (!pipelineOptional)
		{
			message = "No pipeline found for image name: " + pipelineName;
			std::println("{}", message);
			return std::unexpected(message);
		}

		auto& pipeline = pipelineOptional->get();
		auto process = processPipeline(pipeline, path);
		if (!process)
		{
			message = "Pipeline process failed: " + pipelineName;
			std::println("{}", message);
			return std::unexpected(message);
		}

		message = "Pipeline process finished: " + pipelineName;
		std::println("{}", message);
		return message;
	}
	auto runAllPipelines(const std::filesystem::path& path = {}) -> std::expected<std::string, std::string>
	{ //TODO Paths...
		auto message = std::string{};
		std::println("Starting pipelines...");

		auto futures = std::vector<std::pair<std::string, std::future<std::expected<void, ErrorType>>>>{};
		for (auto& pipeline : m_pipelines)
		{
			futures.push_back(std::make_pair(pipeline.input.getImageName(), std::async(std::launch::async, [this, &pipeline]() {
				return this->processPipeline(pipeline);
				})));
		}
		for (auto& f : futures)
		{
			auto result = f.second.get();
			if (!result)
				message += "Pipeline process failed: " + f.first;
			else
				message += "Pipeline process finished: " + f.first;
		}

		std::println("{}", message);
		return message;
	}
protected:
public:
	auto _run() -> std::expected<void, ErrorType> //TODO move printlns... here.
	{
		/*TODO*/
		auto line = std::string{};
		std::println("Welcome to ImageLab!");
		std::println("Commands: ");
		std::println("load <path>");
		std::println("add-filter <img_name> <type> [param]");
		std::println("show <img_name>");
		std::println("show-all");
		std::println("run <img_name>");
		std::println("run-all");
		std::println("exit");

		while (true)
		{
			std::print("> ");
			if (!std::getline(std::cin, line) || line == "exit")
				break;

			auto ss = std::stringstream{ line };
			auto command = std::string{};
			ss >> command;

			if (command == "load")
			{
				auto path = std::string{};
				ss >> path;
				loadImage(path);
			}
			else if (command == "add-filter")
			{
				auto img_name = std::string{};
				auto type = std::string{};
				ss >> img_name >> type;
				auto param = float32_t{};
				if (ss >> param)
				{
				}
				addFilter(img_name, type, param);
			}
			else if (command == "remove-filter")
			{
				auto img_name = std::string{};
				auto index = size_t{};
				ss >> img_name >> index;

				removeFilter(img_name, index);
			}
			else if (command == "show")
			{
				auto img_name = std::string{};
				ss >> img_name;
				listPipeline(img_name);
			}
			else if (command == "show-all")
			{
				listAllPipelines();
			}
			else if (command == "run")
			{
				auto img_name = std::string{};
				ss >> img_name;
				runPipeline(img_name);
			}
			else if (command == "run-all")
			{
				runAllPipelines();
			}
			else if (!command.empty())
			{
				//...
			}
		}
		std::println("Exiting...");
		return {};
	}
};

#endif // !IAPP_H
