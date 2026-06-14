#ifndef FILTER_PIPELINE_HPP

#define FILTER_PIPELINE_HPP

#include"Utility.hpp"
#include"Image.h"
#include"Filter.hpp"
#include"Filters.hpp"

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

	auto listFilterInfos() const -> std::vector<std::string>
	{
		auto filterInfos = std::vector<std::string>{};
		filterInfos.reserve(m_filterOrder.size());

		for (const auto& filterRef : m_filterOrder)
		{
			std::visit([&filterInfos](auto& filter) {
				filterInfos.push_back(filter.get().info());
				}, filterRef);
		}
		auto counter = size_t{ 0 };
		for (auto& i : filterInfos)
		{
			i = std::format("[{}]-{}", ++counter, i);
		}
		return filterInfos;
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

					if (std::holds_alternative<std::reference_wrapper<FilterType>>(filterRef) &&
						std::holds_alternative<std::reference_wrapper<FilterType>>(removedFilterRef))
					{
						filterRef = std::ref(vectors[vectorIndexes[typeIndex]++]);
					}
					else if (std::holds_alternative<std::reference_wrapper<FilterType>>(filterRef))
					{
						vectorIndexes[typeIndex]++;
					}
					++typeIndex;
					}()), ...);
				}, m_filters);
		}

		return removedFilter;
	}
};

using TFilterPipeline = FilterPipeline
<
	InversionFilter,
	ContrastNormalizationFilter,
	SharpenFilter,
	BlurFilter,
	GrayscaleFilter,
	SobelEdgeDetectionFilter
>;

#endif // !FILTER_PIPELINE_HPP