#ifndef CONSOLE_APP_H

#define CONSOLE_APP_H

#include"Utility.hpp"
#include"Image.h"
#include"Filter.hpp"
#include"Filters.hpp"
#include"FilterPipeline.hpp"
#include"IApp.hpp"

class ConsoleApp : public IApp
{
private:

	static constexpr std::string_view HELP =
		"load <path> [img_name]                 Load an image into a pipeline.\n"
		"add-filter <img_name> <type> [params...]\n"
		"remove-filter <img_name> <index>       Remove a filter by its 1-based index.\n"
		"show <img_name>                        List all filters active on an image.\n"
		"show-all                               List filters across all pipelines.\n"
		"run <img_name> [path]                  Process and save a specific image.\n"
		"run-all                                Process and save all active images.\n"
		"help                                   Display this help interface.\n"
		"clear                                  Clear the console screen.\n"
		"reset									Resets the state of the application.\n"
		"exit                                   Terminate the application.\n"
		"\n"
		"  Available Filter Types:\n"
		"    - inversion                        No parameters required.\n"
		"    - contrast-normalization           No parameters required.\n"
		"    - sharpen <sharpness>              Takes a real number (e.g., 1.6).\n"
		"    - blur <kernel_size>               Takes an odd integer >= 3 (e.g., 5).\n"
		"    - grayscale						No parameters required.\n"
		"    - sobel <enabled> <threshold>      Takes a bool (0/1) and an integer (0-255).\n";

	auto loadImage(const std::string& path, const std::string& name = {}) -> std::expected<void, ErrorType>
	{
		auto message = std::string{};

		auto image = Image{};
		auto loadResult = image.loadNetpbm(path);
		if (!loadResult)
		{
			message = "Failed to load image: " + path;
			std::println("{}.", message);
			return std::unexpected(loadResult.error());
		}

		if (!name.empty())
			image.setCustomImageName(name);
		auto usedName = image.getImageName();
		if (getPipeline(usedName))
		{
			message = "Image with name '" + usedName + "' already exists in the pipeline";
			std::println("{}.", message);
			return std::unexpected(ErrorType::DuplicateImagePipeline);
		}

		m_pipelines.push_back(ImagePipiline{ std::move(image), {}, Image{} });
		message = "Image loaded successfully: " + path;
		std::println("{}.", message);
		return {};

	}
	auto addFilter(const std::string& pipelineName, const std::string& filterType, const FilterArgs& args) -> std::expected<void, ErrorType>
	{
		auto message = std::string{};

		auto pipelineOptional = getPipeline(pipelineName);
		if (!pipelineOptional)
		{
			message = "No pipeline found for image: " + pipelineName;
			std::println("{}.", message);
			return std::unexpected(ErrorType::NoImagePipeline);
		}

		auto& pipeline = pipelineOptional->get();
		if (filterType == "inversion")
		{
			auto result = pipeline.filters.addFilter(InversionFilter{});
			if (!result)
				return std::unexpected(result.error());
		}
		else if (filterType == "contrast-normalization")
		{
			auto result = pipeline.filters.addFilter(ContrastNormalizationFilter{});
			if (!result)
				return std::unexpected(result.error());
		}
		else if (filterType == "sharpen")
		{
			auto g = std::get<SharpenFilterArgs>(args);
			auto result = pipeline.filters.addFilter(SharpenFilter{ g });
			if (!result)
				return std::unexpected(result.error());
		}
		else if (filterType == "blur")
		{
			auto g = std::get<BlurFilterArgs>(args);
			auto result = pipeline.filters.addFilter(BlurFilter{ g });
			if (!result)
				return std::unexpected(result.error());
		}
		else if (filterType == "grayscale")
		{
			auto result = pipeline.filters.addFilter(GrayscaleFilter{});
			if (!result)
				return std::unexpected(result.error());
		}
		else if (filterType == "sobel")
		{
			auto g = std::get<SobelFilterArgs>(args);
			auto result = pipeline.filters.addFilter(SobelEdgeDetectionFilter{ g });
			if (!result)
				return std::unexpected(result.error());
		}
		else
		{
			message = "Unknown filter type: " + filterType;
			std::println("{}.", message);
			return std::unexpected(ErrorType::InvalidFilter);
		}

		message = "Filter added successfully to image: " + pipelineName;
		std::println("{}.", message);
		return {};
	}
	auto removeFilter(const std::string& pipelineName, size_t filterIndex) -> std::expected<void, ErrorType>
	{
		auto message = std::string{};

		auto pipelineOptional = getPipeline(pipelineName);
		if (!pipelineOptional)
		{
			message = "No pipeline found for image: " + pipelineName;
			std::println("{}.", message);
			return std::unexpected(ErrorType::NoImagePipeline);
		}

		auto& pipeline = pipelineOptional->get();
		auto removeResult = pipeline.filters.removeFilter(filterIndex);
		if (!removeResult)
		{
			message = "Failed to remove filter at index " + std::to_string(filterIndex + 1) + " from image: " + pipelineName;
			std::println("{}.", message);
			return std::unexpected(removeResult.error());
		}

		message = "Filter removed successfully from image: " + pipelineName;
		std::println("{}.", message);
		return {};
	}
	auto listPipeline(const std::string& pipelineName) const -> std::expected<void, ErrorType>
	{
		auto message = std::string{};

		auto pipelineOptional = getPipeline(pipelineName);
		if (!pipelineOptional)
		{
			message = "No pipeline found for image: " + pipelineName;
			std::println("{}.", message);
			return std::unexpected(ErrorType::NoImagePipeline);
		}

		auto& pipeline = pipelineOptional->get();
		auto filterList = pipeline.filters.listFilterInfos();

		if (filterList.empty())
		{
			message = "No filters found in pipeline for image: " + pipelineName;
			std::println("{}.", message);
			return {};
		}

		message = "Filters in pipeline for image " + pipelineName + ": \n";
		for (const auto& filterName : filterList)
			message += filterName + ", ";
		message.pop_back(); message.pop_back();
		std::println("{}", message); //ex.
		return {};
	}
	auto listAllPipelines() const -> std::expected<void, ErrorType>
	{
		auto message = std::string{};
		if (m_pipelines.empty())
		{
			message = "No pipelines found";
			std::println("{}.", message);
			return std::unexpected(ErrorType::NoImagePipeline);
		}

		message = {};
		auto flag = false;
		for (const auto& pipeline : m_pipelines)
		{
			auto pipelineList = listPipeline(pipeline.input.getImageName());
			if (pipelineList)
			{
				flag = true;
			}
			else
			{
			}
		}

		return {};
	}

	auto runPipeline(const std::string& pipelineName, const std::filesystem::path& path = {}) -> std::expected<void, ErrorType>
	{
		auto message = std::string{};
		message = "Starting pipeline for image: " + pipelineName;
		std::println("{}.", message);

		auto pipelineOptional = getPipeline(pipelineName);
		if (!pipelineOptional)
		{
			message = "No pipeline found for image: " + pipelineName;
			std::println("{}.", message);
			return std::unexpected(ErrorType::NoImagePipeline);
		}

		auto& pipeline = pipelineOptional->get();
		auto process = processPipeline(pipeline, path);
		if (!process)
		{
			message = "Pipeline process failed: " + pipelineName;
			std::println("{}.", message);
			return std::unexpected(process.error());
		}

		message = "Pipeline process finished: " + pipelineName;
		std::println("{}.", message);
		return {};
	}
	auto runAllPipelines() -> std::expected<void, ErrorType>
	{
		auto message = std::string{};
		if (m_pipelines.empty())
		{
			message = "No pipelines found";
			std::println("{}.", message);
			return std::unexpected(ErrorType::NoImagePipeline);
		}
		std::println("Starting pipelines...");

		auto futures = std::vector<std::pair<std::string, std::future<std::expected<void, ErrorType>>>>{};
		for (auto& pipeline : m_pipelines)
		{
			futures.push_back(std::make_pair(pipeline.input.getImageName(), std::async(std::launch::async, [this, &pipeline]() {
				return this->runPipeline(pipeline.input.getImageName());
				})));
		}
		auto flag = false;
		for (auto& f : futures)
		{
			auto result = f.second.get();
			if (!result)
				;
			else
				flag = true;
		}

		return {};
	}
protected:
public:
	auto _run() -> std::expected<void, ErrorType>
	{
		auto line = std::string{};
		std::println("Welcome to ImageLab!");
		std::println("Enter help to get the list of commands...");

		while (true)
		{
			std::print("> ");
			if (!std::getline(std::cin, line))
				break;

			line = trim(line);
			if (line.empty())
				continue;
			if (line == "exit")
				break;

			auto ss = std::stringstream{ line };
			auto command = std::string{};
			ss >> command;
			command = trim(command);

			if (command == "load")
			{
				auto path = std::string{};
				if (ss >> path)
				{
					auto name = std::string{};
					ss >> name;

					path = trim(path);
					name = trim(name);

					auto result = loadImage(path, name);
				}
				else
				{
					std::println("Syntax of load: load <path> [img_name]");
				}
			}
			else if (command == "add-filter")
			{
				auto img_name = std::string{};
				auto type = std::string{};
				if (ss >> img_name >> type)
				{
					auto args = FilterArgs{ std::monostate{} };

					img_name = trim(img_name);
					type = trim(type);

					if (type == "sharpen")
					{
						auto a = SharpenFilterArgs{};
						if (ss >> a.sharpness) {}
						args = a;
					}
					else if (type == "blur")
					{
						auto a = BlurFilterArgs{};
						if (ss >> a.kernelSize) {}
						args = a;
					}
					if (type == "sobel")
					{
						auto a = SobelFilterArgs{};
						if (ss >> a.thresholdEnabled >> a.threshold) {}
						args = a;
					}

					auto result = addFilter(img_name, type, args);
				}
				else
				{
					std::println("Syntax of add-filter: add-filter <img_name> <type> [params...]");
				}

			}
			else if (command == "remove-filter")
			{
				auto img_name = std::string{};
				auto index = size_t{};
				if (ss >> img_name >> index)
				{
					img_name = trim(img_name);
					--index;

					auto result = removeFilter(img_name, index);
				}
				else
				{
					std::println("Syntax of remove-filter: remove-filter <img_name> <index>");
				}

			}
			else if (command == "show")
			{
				auto img_name = std::string{};
				if (ss >> img_name)
				{
					img_name = trim(img_name);

					auto result = listPipeline(img_name);
				}
				else
				{
					std::println("Syntax of show: show <img_name>");
				}

			}
			else if (command == "show-all")
			{
				auto result = listAllPipelines();
			}
			else if (command == "run")
			{
				auto img_name = std::string{};
				if (ss >> img_name)
				{
					auto path = std::string{};
					ss >> path;

					img_name = trim(img_name);
					path = trim(path);

					auto result = runPipeline(img_name, path);
				}
				else
				{
					std::println("Syntax of run: run <img_name>");
				}
			}
			else if (command == "run-all")
			{
				auto result = runAllPipelines();
			}
			else if (command == "help")
			{
				std::print("{}", HELP);
			}
			else if (command == "clear")
			{
				std::print("\033[H\033[2J");
				std::flush(std::cout);
			}
			else if (command == "reset")
			{
				m_pipelines.clear();
				std::print("\033[H\033[2J");
				std::flush(std::cout);
				std::println("Welcome to ImageLab!");
				std::println("Enter help to get the list of commands...");
			}
			else if (!command.empty())
			{
				//...
				auto message = std::string{};
				message = "Unknown command";
				std::println("{}.", message);
			}
		}
		std::println("Goodbye!");
		return {};
	}
};

#endif // !CONSOLE_APP_H
