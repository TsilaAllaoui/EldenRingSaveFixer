#include "savefile.h"
#include <util.h>
#include <fstream>
#include <span>
#include <string_view>

void Slot::copy(SaveSpan source, SaveSpan target, size_t targetSlotIndex) const {
	const auto targetSlotSection{ ParseSlot(SlotSectionOffset, SlotSection.size, targetSlotIndex) };
	const auto targetHeaderSection{ ParseHeader(SlotHeaderSectionOffset, SlotHeaderSection.size, targetSlotIndex) };
	targetSlotSection.replace(target, SlotSection.bytesFrom(source));
	targetHeaderSection.replace(target, SlotHeaderSection.bytesFrom(source));
}

void Slot::debugListItems(SaveSpan data) {
	const auto slot{ SlotSection.bytesFrom(data) };
	std::vector<Items::ItemResult> recognized{};
	std::vector<Items::ItemResult> unknown{};
	Items::Items known;

	for (auto itr{ slot.begin() }; itr + 2 != slot.end(); itr++) {
		if (*itr == Items::ItemDelimiter.front() && *(itr + 1) == Items::ItemDelimiter.back()) [[unlikely]] {
			const Items::ItemResult item{ static_cast<size_t>(itr - slot.begin()), {*(itr - 2), *(itr - 1)} };
			const auto group{ known.hasGroup(item) };
			const auto quantity{ getItemQuantity(data, item.item) };
			if (!quantity) // Probably isnt an item
				continue;

			if (group.found && known.findId(item).empty()) // Ignore items we already know
				recognized.emplace_back(item, group.name, quantity);
			else
				unknown.emplace_back(item, quantity);
			}
		else if (*(itr + 1) != Items::ItemDelimiter.front()) [[likely]]
			itr++;
	}

	//// TODO: this sometimes doesnt find all duplicates, no idea why
	//for (auto checking{ unknown.begin() }; checking != unknown.end(); checking++) {
	//	for (auto pos{ checking }; pos + 1 != unknown.end(); pos++) {
	//		auto duplicate{ std::find_if(pos, unknown.end(), [&checking](const Items::ItemResult& cmp) {
	//			if (cmp.item.group == checking->item.group && cmp.item.id == checking->item.id && checking->quanity == cmp.quanity)
	//				return true;
	//			return false;
	//		}) };

	//		if (duplicate == unknown.end())
	//			break;

	//		checking->insertDuplicate(duplicate->offset);
	//		unknown.erase(duplicate);
	//	}
	//}

	if (!unknown.empty()) {
		fmt::print("found {} unique unknown items:\n\n", unknown.size());
		for (auto& result : unknown) {
			if (!result.duplicates.empty())
				fmt::print("\n");
			fmt::print("0x{:06X}: group: {:02X}, id: {:02X}, quanity: {}\n", result.offset, result.item.group, result.item.id, result.quanity);
			if (!result.duplicates.empty()) {
				for (auto& dupe : result.duplicates)
					fmt::print("    duplicate at 0x{:06X}\n", dupe);
				fmt::print("\n");
			}
		}
	}

	if (!recognized.empty()) {
		fmt::print("\nfound {} unknown items with a recognized group:\n\n", recognized.size());
		for (auto& result : recognized)
			fmt::print("0x{:06X}: {}, id: {:02X}, quanity: {}\n", result.offset, result.name, result.item.id, result.quanity);
	}
}

void Slot::recalculateSlotChecksum(SaveSpan data) const {
	auto hash{ util::GenerateMd5(SlotSection.bytesFrom(data)) };
	SlotChecksumSection.replace(data, hash);
}

void Slot::rename(SaveSpan data, std::string_view newName) const {
	std::array<u8, NameSectionSize> convertedName{};
	util::Utf8ToUtf16(convertedName, std::u16string(newName.begin(), newName.end()));
	// Any characters sharing the same name will get replaced with the new name as of now
	util::ReplaceAll<u8>(data, NameSection.bytesFrom(data), convertedName);
}

u32 Slot::getItemQuantity(SaveSpan data, Items::Item item) const {
	auto slot{ SlotSection.bytesFrom(data) };
	const auto itr{ std::search(slot.begin(), slot.end(), item.data.begin(), item.data.end()) };
	return (itr != slot.end()) ? slot[itr - slot.begin() + item.data.size()] : 0;
}

void Slot::setItemQuantity(SaveSpan data, Items::Item item, u32 quantity) const {
	constexpr static auto itemSize{ 10 };
	auto slot{ SlotSection.bytesFrom(data) };
	size_t quantityOffset{};

	if (auto itr = std::search(slot.begin(), slot.end(), item.data.begin(), item.data.end()); itr != slot.end())
		// If the item is already present we can just update the quantity
		quantityOffset = (itr - slot.begin()) + item.data.size();
	else {
		// Otherwise we need to insert it. This currently works, but only for a few items.
		for (size_t i{}; i < slot.size(); i++) {
			// Check if an item exists at the current position
			if (slot[i] == Items::ItemDelimiter.front() && slot[i + 1] == Items::ItemDelimiter.back()) {
				const auto nextItem{ i + itemSize };
				const auto nextItemEnd{ i + (itemSize * 2) };
				// Check if the space after the found item is empty
				if (std::search_n(slot.begin() + nextItem, slot.begin() + nextItemEnd, itemSize, 0x0) != slot.begin() + nextItemEnd) {
					// If yes, copy the requested item into it
					std::copy(item.data.begin(), item.data.end(), slot.begin() + nextItem);
					quantityOffset = nextItem + item.data.size();
					break;
				}
				else
					i += itemSize;
			}
		}
	}

	if (!quantityOffset)
		throw exception("Could not find a place for item with quantity {}", quantity);
	slot[quantityOffset] = static_cast<u8>(quantity);
}

std::string Slot::getName(SaveSpan data) const {
	return util::Utf16ToUtf8String(NameSection.bytesFrom(data));
}

void Slot::setActive(SaveSpan data, bool value) const {
	ActiveSection.bytesFrom(data)[index] = value;
}

std::string Slot::getTimePlayed(SaveSpan data) const {
	return util::SecondsToTimeStamp(SecondsPlayedSection.castInteger<u32>(data));
}

u64 Slot::getLevel(SaveSpan data) const {
	return LevelSection.castInteger<u8>(data);
}

bool Slot::isActive(SaveSpan data, size_t slotIndex) const {
	return static_cast<bool>(ActiveSection.bytesFrom(data)[slotIndex]);
}

void SaveFile::validateData(SaveSpan data, std::string_view target) const {
	if (HeaderBNDSection.stringFrom(data) != "BND" || data.size_bytes() != SaveFileSize)
		throw exception("{} is not a valid Elden Ring save file.", target);
}

u64 SaveFile::steamId() const {
	return SteamIdSection.castInteger<u64>(saveData);
}

void SaveFile::debugListItems(int slotIndex) {
	slots[slotIndex].debugListItems(saveData);
}

std::vector<u8> SaveFile::loadFile(std::filesystem::path path) const {
	std::ifstream file(path, std::ios::in | std::ios::binary);
	std::vector<u8> buffer;
	if (!std::filesystem::exists(path))
		throw exception("Path {} does not exist.", util::ToAbsolutePath(path).generic_string());
	if (!file.is_open())
		throw exception("Could not open file '{}'", util::ToAbsolutePath(path).generic_string());

	uint8_t byte;
	while (file.read(reinterpret_cast<char*>(&byte), sizeof(uint8_t))) {
		buffer.push_back(byte);
	}

	file.close();
	return buffer;
}

void SaveFile::write(SaveSpan data, std::filesystem::path path) const {
	// TODO: Seems like this messes up slot names sometimes? Probably encoding related
	std::ofstream file(path, std::ios::out | std::ios::binary);
	if (!std::filesystem::exists(path))
		throw exception("Path {} does not exist.", util::ToAbsolutePath(path).generic_string());
	if (!file.is_open())
		throw exception("Could not open file '{}'", util::ToAbsolutePath(path).generic_string());

	validateData(data, "Generated data");
	recalculateChecksums(data);
	file.write(reinterpret_cast<const char*>(data.data()), data.size_bytes());
}

const std::vector<Slot> SaveFile::parseSlots(SaveSpan data) const {
	std::vector<Slot> buffer;
	for (size_t i{}; i < SlotCount; i++)
		buffer.push_back(Slot{ data, i });

	return buffer;
}

void SaveFile::refreshSlots() {
	auto newSlots{ parseSlots(saveData) };
	slots.swap(newSlots);
}

void SaveFile::copySlot(SaveFile& source, size_t sourceSlotIndex, size_t targetSlotIndex) {
	if (targetSlotIndex > SlotCount || sourceSlotIndex > SlotCount)
		throw exception("Invalid slot index while copying character");

	source.slots[sourceSlotIndex].copy(source.saveData, saveData, targetSlotIndex);
	replaceSteamId(source.saveData, steamId());
	setSlotActivity(targetSlotIndex, true);
	refreshSlots();
}

void SaveFile::copySlot(size_t sourceSlotIndex, size_t targetSlotIndex) {
	return copySlot(*this, sourceSlotIndex, targetSlotIndex);
}

void SaveFile::appendSlot(SaveFile& source, size_t sourceSlotIndex) {
	size_t firstAvailableSlot{ SlotCount + 1 };
	for (auto& slot : slots)
		if (!slot.active) {
			firstAvailableSlot = slot.index;
			break;
		}

	if (firstAvailableSlot == SlotCount + 1)
		throw exception("Could not find an unactive slot to append slot {} to", sourceSlotIndex);
	copySlot(source, sourceSlotIndex, firstAvailableSlot);
	setSlotActivity(firstAvailableSlot, true);
}

void SaveFile::appendSlot(size_t sourceSlotIndex) {
	return appendSlot(*this, sourceSlotIndex);
}

void SaveFile::renameSlot(size_t slotIndex, std::string_view name) {
	if (slotIndex > SlotCount)
		throw exception("Invalid slot index while renaming character");

	slots[slotIndex].rename(saveData, name);
	refreshSlots();
}

void SaveFile::replaceSteamId(SaveSpan replaceFrom, u64 newSteamId) const {
	std::array<u8, sizeof(u64)> steamIdData{};
	std::memcpy(steamIdData.data(), &newSteamId, sizeof(u64));
	util::ReplaceAll<u8>(saveData, SteamIdSection.bytesFrom(replaceFrom), steamIdData);
}

void SaveFile::replaceSteamId(u64 newSteamId) const {
	replaceSteamId(saveData, newSteamId);
}

void SaveFile::recalculateChecksums(SaveSpan data) const {
	auto saveHeaderChecksum{ util::GenerateMd5(SaveHeaderSection.bytesFrom(data)) };
	SaveHeaderChecksumSection.replace(data, saveHeaderChecksum);
	for (auto& slot : slots)
		slot.recalculateSlotChecksum(data);
}

void SaveFile::setSlotActivity(size_t slotIndex, bool active) {
	slots[slotIndex].setActive(saveData, active);
	refreshSlots();
}

u32 SaveFile::getItem(size_t slot, Items::Item item) const {
	return slots[slot].getItemQuantity(saveData, item);
}

void SaveFile::setItem(size_t slot, Items::Item item, u32 quantity) const {
	slots[slot].setItemQuantity(saveData, item, quantity);
}

void SaveFile::printActiveSlots() const {
	for (auto slot : slots)
		if (slot.active)
			printSlot(slot.index);
}

void SaveFile::printSlot(size_t slotIndex) const {
	const auto slot{ slots[slotIndex] };
	if (!slot.active)
		fmt::print("warning: slot {} is not active\n", slotIndex);
	fmt::print("slot {}: {}, level {}, played for {}\n", slotIndex, slot.name, slot.level, slot.timePlayed);
}

void SaveFile::printItems(size_t slotIndex) const {
	const auto slot{ slots[slotIndex] };
	if (!slot.active)
		fmt::print("warning: slot {} is not active\n", slotIndex);
	for (auto item : items)
		if (getItem(slotIndex, item.second))
			fmt::print("{}: {}\n", item.first, getItem(slotIndex, item.second));
}
