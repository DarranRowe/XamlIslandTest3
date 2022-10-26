#pragma once
#include "IslandApplication.g.h"

namespace winrt::XamlIslandTest3::implementation
{
	//Implementation of the Xaml Application required to load WinUI and other Xaml content.
	struct IslandApplication : IslandApplicationT<IslandApplication, winrt::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider>
	{
		IslandApplication() = default;

		IslandApplication(winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider> const& providers);
		~IslandApplication();

		winrt::Windows::Foundation::IClosable WindowsXamlManager() const;
		bool IsDisposed() const;
		void Initialize();
		winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider> MetadataProviders();
		void Close();

		//Implementation of the IXamlMetadataProvider interface.
		winrt::Microsoft::UI::Xaml::Markup::IXamlType GetXamlType(winrt::Windows::UI::Xaml::Interop::TypeName const &type);
		winrt::Microsoft::UI::Xaml::Markup::IXamlType GetXamlType(winrt::hstring const &fullname);
		winrt::com_array<winrt::Microsoft::UI::Xaml::Markup::XmlnsDefinition> GetXmlnsDefinitions();

	private:

		bool m_isclosed = false;
		winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager m_xamlmanager = nullptr;
		winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider> m_providers = winrt::single_threaded_vector<winrt::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider>();
	};
}
namespace winrt::XamlIslandTest3::factory_implementation
{
	struct IslandApplication : IslandApplicationT<IslandApplication, implementation::IslandApplication>
	{
	};
}
