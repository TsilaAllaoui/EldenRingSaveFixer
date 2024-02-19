#include "util.h"
#include <chrono>
#include <functional>
#include <openssl/evp.h>
#include <span>

namespace util {

	const Md5Hash GenerateMd5(std::span<u8> input) {
		Md5Hash hash{};
		auto context{ EVP_MD_CTX_new() };
		EVP_DigestInit_ex(context, EVP_md5(), nullptr);
		EVP_DigestUpdate(context, input.data(), input.size_bytes());
		EVP_DigestFinal_ex(context, hash.data(), nullptr);
		return hash;
	}

	class Utf8Utf16Converter : public std::codecvt<char16_t, char8_t, std::mbstate_t> {
	public:
		~Utf8Utf16Converter() override = default;
	};

	const std::string Utf16ToUtf8String(std::span<u8> chars) {
		std::string output = "";
		for (auto c : chars) {
			if (c == '\0') continue;
			output += c;
		}
		return output;
	}

	void Utf8ToUtf16(std::span<u8> chars, std::u16string_view text) {
		std::memcpy(chars.data(), text.data(), std::min(text.size() * sizeof(char16_t), chars.size_bytes()));
		if (text.size() * sizeof(char16_t) > chars.size()) // Null terminate if the string if it isnt out of bounds
			*(reinterpret_cast<char16_t*>(chars.data()) + text.size()) = u'\0';
	}

	const std::string SecondsToTimeStamp(const time_t input) {
		constexpr static auto SecondsInHour{ 60 };
		constexpr static auto MinutesInHour{ SecondsInHour * 60 };
		const auto hours{ std::chrono::duration_cast<std::chrono::hours>(std::chrono::seconds(input)) };
		const auto minutes{ std::chrono::duration_cast<std::chrono::minutes>(std::chrono::seconds(input - (hours.count() * MinutesInHour))) };
		const auto seconds{ std::chrono::duration_cast<std::chrono::seconds>(std::chrono::seconds(input - (hours.count() * MinutesInHour) - (minutes.count() * SecondsInHour))) };

		return fmt::format("{:02}:{:02}:{:02}", hours.count(), minutes.count(), seconds.count());
	}

	const std::filesystem::path ToAbsolutePath(std::filesystem::path path) {
		return std::filesystem::absolute(path);
	}

	const std::string GetEnvironmentVariable(std::string_view name, std::function<std::string()> defaultValue) {
		return "";// { (std::getenv(name.data()) != nullptr) ? std::getenv(name.data()) : defaultValue.operator()() };
	}

	const std::string GetEnvironmentVariable(std::string_view name, std::string_view defaultValue) {
		return GetEnvironmentVariable(name, [defaultValue]() {
			return defaultValue.data();
			});
	}

	Maybe<std::filesystem::path> FindFileInSubDirectory(std::filesystem::path directory, std::string_view filename) {
		if (std::filesystem::exists(directory)) {
			for (auto& path : std::filesystem::directory_iterator(directory))
				if (path.is_directory()) {
					const auto filePath{ path.path() / filename };
					if (std::filesystem::exists(filePath))
						return filePath;
				}
		}

		return fmt::format("Could not find file '{}' in any subdirectory of '{}'", filename, directory.string());
	}

	u64 GetSteamId(std::filesystem::path saveFilePath) {
		u64 steamId;
		try {
			// Folder structure is 'EldenRing/<Steam ID>/ER0000.sl2'
			steamId = static_cast<u64>(std::stoull(saveFilePath.parent_path().filename().generic_string()));
		}
		catch (std::exception& e) {
			exception("Failed to parse Steam ID: {}", e.what());
		}

		return steamId;
	}

	std::filesystem::path CreateDataDirectory() {
		/*std::filesystem::path directory{ GetEnvironmentVariable("XDG_DATA_HOME", []() -> std::filesystem::path {
			auto home{GetEnvironmentVariable("HOME")};
			if (!home.empty())
				return std::filesystem::path(home) / ".config";
			return std::filesystem::current_path();
		}) };

		directory /= "er-saveutils";
		if (!std::filesystem::exists(directory))
			std::filesystem::create_directory(directory);
		return directory;*/
		return std::filesystem::path("");
	}

	std::filesystem::path CreateBackupDirectory() {
		//auto directory{ CreateDataDirectory() / "backup" };
		//const auto timePoint{ std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
		//auto timeStamp{ fmt::format("{}", std::ctime(&timePoint)) }; // TODO: format this in a better way
		//timeStamp.pop_back();                                      // remove newline, thanks ctime
		//if (!std::filesystem::exists(directory))
		//	std::filesystem::create_directory(directory);

		//directory /= timeStamp;
		//if (!std::filesystem::exists(directory))
		//	std::filesystem::create_directory(directory);
		//return directory;

		return std::filesystem::path("");
	}

	std::filesystem::path BackupSavefile(std::filesystem::path saveFilePath) {
		auto backupDir{ util::CreateBackupDirectory() };
		std::filesystem::path bakFilePath{ saveFilePath.string() + ".bak" };
		if (std::filesystem::exists(saveFilePath))
			std::filesystem::copy(saveFilePath, backupDir / saveFilePath.filename());
		if (std::filesystem::exists(bakFilePath)) {
			std::filesystem::copy(bakFilePath, backupDir / bakFilePath.filename());
			std::filesystem::remove(bakFilePath); // If this differentiates from ER0000.sl2 the game will claim the savefile is corrupt
		}

		return backupDir;
	}

} // namespace util
