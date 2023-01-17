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

class Logger
{
public:
	static void Log(const std::string& text)
	{
		std::clog << text << "\n";
	}

	static void LogColorful(const std::string& text, unsigned short color)
	{
		auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		SetConsoleTextAttribute(hConsole, color);
		std::clog << text << "\n";
		SetConsoleTextAttribute(hConsole, 0x7); // Restore color.
	}
};

class Patcher
{
	// Credits to Kamzik123 for finding the offsets.
	std::array<GameData, 3> Games =
	{
		GameData("Assassin's Creed III", "AC3SP.exe", 0xBF2CC, 0x86, 0x8E),
		GameData("Assassin's Creed IV Black Flag", "AC4BFSP.exe", 0x5B602, 0x86, 0x8E),
		GameData("Assassin's Creed Rogue", "ACC.exe", 0x9CDDC, 0x0, 0x01)
	};

	bool Patch(std::fstream& gameFile, int gameId, bool enableWindowMode = true)
	{
		// Get the size of the game executable file
		gameFile.seekg(0, gameFile.end);
		long long size = gameFile.tellg();

		// Allocate a buffer to hold the contents
		char* buffer = new char[size];

		// Set the stream back to the beginning of the file
		gameFile.seekg(0, gameFile.beg);

		// Read the entire file into the buffer
		gameFile.read(buffer, size);

		// Read the current state, true if WM enabled, false otherwise
		bool currentState = (*((unsigned char*)(buffer + Games[gameId].WindowModeOffset)) == Games[gameId].EnabledByte);

		// Do not override the file if it is in the correct state
		if (currentState != enableWindowMode)
		{
			if (!enableWindowMode)
			{
				// Disable the windowed mode
				*((unsigned char*)(buffer + Games[gameId].WindowModeOffset)) = Games[gameId].DisabledByte;
				Logger::LogColorful("Disabled windowed mode", 0xC);
			}
			else
			{
				// Enable the windowed mode
				*((unsigned char*)(buffer + Games[gameId].WindowModeOffset)) = Games[gameId].EnabledByte;

				Logger::LogColorful("Enabled windowed mode", 0xA);
			}

			// Write the modified bytes
			gameFile.seekg(0, gameFile.beg);
			gameFile.write(buffer, size);
		}

		// Delete the buffer
		delete[] buffer;
		return true;
	}

	bool StartProcess(const std::string& fileName)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));

		si.cb = sizeof(si);

		// Start the process. 
		if (CreateProcess(fileName.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			// Close handles. 
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			return true;
		}

		return false;
	}

	void OnGameNotFound()
	{
		std::cout << "Game executable not found, make sure to run this program in the game directory.\n";
		std::cout << "The patchers supports the following games:\n\n";

		for (unsigned int gameId = 0; gameId < Games.size(); gameId++)
		{
			std::cout << gameId + 1 << ". " << Games[gameId].GameName << "\n";
		}
	}

public:
	bool TryPatch(bool enableWindowMode = true, bool startProcess = false)
	{
		bool gameFound = false;

		for (unsigned int gameId = 0; gameId < Games.size(); gameId++)
		{
			std::fstream gameFile(Games[gameId].GameExecutableName, std::ios::in | std::ios::out | std::ios::binary);

			if (gameFile.is_open())
			{
				gameFound = true;
				Logger::Log("Found " + Games[gameId].GameName);

				Patch(gameFile, gameId, enableWindowMode);
				gameFile.close();

				if (startProcess)
				{
					StartProcess(Games[gameId].GameExecutableName);
				}

				return true;
			}
		}

		if (!gameFound)
		{
			OnGameNotFound();
		}

		return false;
	}
};

int main(int argc, char** argv)
{
	if (argc == 2)
	{
		Patcher p;

		if (!strcmp(argv[1], "--patch"))
		{
			p.TryPatch(true);
		}
		else if (!strcmp(argv[1], "--restore"))
		{
			p.TryPatch(false);
		}
		else if (!strcmp(argv[1], "--run-windowed"))
		{
			p.TryPatch(true, true);
		}
		else if (!strcmp(argv[1], "--run-fullscreen"))
		{
			p.TryPatch(false, true);
		}
	}
	else
	{
		Logger::Log("Use one of the following: --patch | --restore | --run-windowed | --run-fullscreen");
	}

	return 0;
}
