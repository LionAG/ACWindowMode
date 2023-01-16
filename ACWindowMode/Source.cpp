#include <Windows.h>

#include <iostream>
#include <fstream>
#include <string>
#include <array>

struct GameData
{
	std::string GameName;
	std::string GameExecutableName;
	uintptr_t WindowModeOffset;
	unsigned char EnabledByte;	// Windowed mode ON
	unsigned char DisabledByte; // Windowed mode OFF

	GameData(const std::string& gameName, const std::string& gameExecutableName, uintptr_t windowModeOffset, unsigned char enabledByte, unsigned char disabledByte)
	{
		GameName = gameName;
		GameExecutableName = gameExecutableName;
		WindowModeOffset = windowModeOffset;
		EnabledByte = enabledByte;
		DisabledByte = disabledByte;
	}
};

void Log(const std::string& text)
{
	std::clog << text << "\n";
}

void LogColorful(const std::string& text, unsigned short color)
{
	auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(hConsole, color);

	std::clog << text << "\n";

	SetConsoleTextAttribute(hConsole, 0x7); // Restore color.
}

int main()
{
	const unsigned int gameCount = 2;

	std::array<GameData, gameCount> Games =
	{
		GameData("Assassin's Creed III", "AC3SP.exe", 0xBF2CC, 0x86, 0x8E),
		GameData("Assassin's Creed IV Black Flag", "AC4BFSP.exe", 0x5B602, 0x86, 0x8E)
	};

	bool gameFound = false;

	for (unsigned int gameId = 0; gameId < Games.size(); gameId++)
	{
		std::fstream gameFile(Games[gameId].GameExecutableName, std::ios::in | std::ios::out | std::ios::binary);

		if (gameFile.is_open())
		{
			gameFound = true;
			Log("Found " + Games[gameId].GameName);

			// Get the size of the game executable file
			gameFile.seekg(0, gameFile.end);
			int size = gameFile.tellg();

			// Allocate a buffer to hold the contents
			char* buffer = new char[size];

			// Set the stream back to the beginning of the file
			gameFile.seekg(0, gameFile.beg);

			// Read the entire file into the buffer
			gameFile.read(buffer, size);

			// Read the current state, true if WM enabled, false otherwise
			bool windowedState = (*((unsigned char*)(buffer + Games[gameId].WindowModeOffset)) == Games[gameId].EnabledByte);
			
			if (windowedState)
			{
				// Disable the windowed mode
				*((unsigned char*)(buffer + Games[gameId].WindowModeOffset)) = Games[gameId].DisabledByte;
				LogColorful("Disabled windowed mode", 0xA);
			}
			else
			{
				// Enable the windowed mode
				*((unsigned char*)(buffer + Games[gameId].WindowModeOffset)) = Games[gameId].EnabledByte;

				LogColorful("Enabled windowed mode", 0xA);
			}
			
			// Write the modified bytes
			gameFile.seekg(0, gameFile.beg);
			gameFile.write(buffer, size);

			// Delete the buffer
			delete[] buffer;

			// Explicitly close the file for clarity. This could be ommited because the object's destructor
			// will close the file when it gets out of scope.
			gameFile.close();
		}
	}

	if (!gameFound)
	{
		std::cout << "Game executable not found, make sure to run this program in the game directory.\n";
		std::cout << "The following games are supported:\n\n";

		for (unsigned int gameId = 0; gameId < Games.size(); gameId++)
		{
			std::cout << gameId + 1 << ". " << Games[gameId].GameName << "\n";
		}
	}

	std::cout << "\nPress any key to close the program.";
	std::cin.get();

	return 0;
}
