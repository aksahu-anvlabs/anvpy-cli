#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include "json.hpp"
#include <unistd.h>
#include <algorithm>
#include <filesystem>
 
using namespace std;
using json = nlohmann::json;
namespace fs = filesystem;

vector<string> modes = {"Console", "Web", "Kivy", "PyGame"};
string cp;
json tmpData = {};

std::string getExecutablePath() {

#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    return buf;

#elif __APPLE__
    char buf[1024];
    uint32_t size = sizeof(buf);
    _NSGetExecutablePath(buf, &size);
    return buf;

#else
    char buf[1024];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (len != -1) {
        buf[len] = '\0';
        return buf;
    }
    return "";
#endif
}

string readFile(string path) {
    ifstream file(path);
    if (!file) return "";

    ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

vector<string> splitLines(const string& data) {
    vector<string> lines;
    stringstream ss(data);
    string line;

    while (getline(ss, line, '\n')) {
        lines.push_back(line);
    }

    return lines;
}

string exec(const string& command) {
	char buffer[128];
	string result = "";
	
	FILE* pipe = popen((command + " 2>/dev/null").c_str(), "r");
    if (!pipe) {
		return "popen failed!";
	}

	while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
		result += buffer;
	}

	int status = pclose(pipe);
	if(status == -1){
		return "Error closing the pipe!";
    }

    return result;
}

long long getLastModifiedUnix(const fs::path& p) {
    auto ftime = fs::last_write_time(p);

    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now()
        + std::chrono::system_clock::now()
    );

    return std::chrono::system_clock::to_time_t(sctp);
}

fs::path normalize_path(const fs::path& p) {
    return fs::relative(p.lexically_normal(), ".");
}

string normalize_string(const fs::path& p) {
    return normalize_path(p).generic_string(); 
}


void scanFolder(const fs::path& path, json& obj, const string& rel) {

    for (const auto& entry : fs::directory_iterator(path)) {

        fs::path relPath = normalize_path(entry.path());
        string relStr = relPath.generic_string();

        string rf = exec(
            "/opt/anv-cli/adb shell ls /sdcard/Android/media/org.anvlabs.anvpy/" 
            + cp + "/" + rel
        );

        auto files = splitLines(rf);
        bool chk = find(
            files.begin(), files.end(),
            entry.path().filename().string()
        ) != files.end();

        if (entry.is_directory()) {

            string nextRel = rel + entry.path().filename().string() + "/";

            if (!chk) {
                string cmd =
                    "/opt/anv-cli/adb shell mkdir -p "
                    "/sdcard/Android/media/org.anvlabs.anvpy/"
                    + cp + "/" + nextRel;
                exec(cmd);
            }

            scanFolder(entry.path(), obj, nextRel);
        }
        else {
            string key = rel + entry.path().filename().string();
            long long mtime = getLastModifiedUnix(entry.path());

            if (!obj.contains(key) || obj[key] != mtime || !chk) {
                string cmd =
                    "/opt/anv-cli/adb push \"" + normalize_string(entry.path()) +
                    "\" \"/sdcard/Android/media/org.anvlabs.anvpy/"
                    + cp + "/" + key + "\"";
                exec(cmd);
            }

            tmpData[key] = mtime;
        }
    }
}


int main(int argc, char* argv[]){
	if(argc == 1){
		cout << "\nAnvPy Desktop v1.1\n\nMake sure to install the official AnvPy X app from Google play on your Android phone before using it. https://play.google.com/store/apps/details?id=org.anvlabs.anvpy \n" << endl;
		cout << "  Steps for using:\n    1. Install AnvPy X on your phone and connect it to desktop via USB/WIFI.\n    2. Enable USB Debugging on your device and check through /opt/anv-cli/adb if available.\n    3. Create new project and check the output on your phone after running it.\n\n";
		cout << "  cmd:\n    create: create a new project <name> <mode>\n    run: run project on AnvPy X.\n    logs: check logs and errors for your program.\n" << std::endl;
	}
	else{
		vector<string> args;
		for (int i = 0; i < argc; ++i) {
		    args.emplace_back(argv[i]);
		}
		if(args[1] == "create"){
			if(argc >= 4){
				if(find(modes.begin(), modes.end(), args[3]) == modes.end()){
					cout << "Invalid project type." << endl;
					return 0;
				}
				if(!fs::exists(args[2])){
					fs::create_directory(args[2]);
					ofstream config(args[2] + "/.config");
					//ofstream deps(args[2] + "/deps.json");
					if(config.is_open()){
						json obj = {
							{"n", args[2] + "~" + args[3]}
						};
						config << obj.dump();
						config.close();
						//deps << "[]";
						//deps.close();
						fs::path exe_path = fs::read_symlink("/proc/self/exe").parent_path();
						fs::path source = "/opt/anv-cli/template/" + args[3] + ".py";
    					fs::path destination = args[2] + "/main.py";
    					fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
    					source = "/opt/anv-cli/template/.manifest";
    					destination = args[2] + "/.manifest";
    					fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
						cout << "Project created successfully" << endl;
					}
				}
				else{
					cout << "Project already exists" << endl;
				}
			}
			else{
				cout << "Invalid arguments" << endl;
			}
		}
		else if(args[1] == "run"){
			if(fs::exists(".config")){
				json obj = json::parse(readFile(".config"));
				cp = obj["n"];
				scanFolder(fs::path("."), obj["data"], "");
				obj["data"] = tmpData;
				ofstream config(".config");
				if(config.is_open()){
					config << obj.dump();
					config.close();
				}
				ofstream anv(".anv");
				if(anv.is_open()){
					anv << cp;
					anv.close();
				}
				exec("/opt/anv-cli/adb push .anv /sdcard/Android/media/org.anvlabs.anvpy/");
				exec("/opt/anv-cli/adb shell am force-stop org.anvlabs.anvpy");
				exec("/opt/anv-cli/adb shell am start -n org.anvlabs.anvpy/.MainActivity -e anv scan");
				cout << "Program started" << endl;
			}
			else{
				cout << "Invalid project" << endl;
			}
		}
		else if(args[1] == "logs"){
			if(fs::exists(".config")){
				json obj = json::parse(readFile(".config"));
				cp = obj["n"];
				string rf = exec("/opt/anv-cli/adb shell cat /sdcard/Android/data/org.anvlabs.anvpy/files/error.txt");
				printf("\033[31m%s\033[0m\n", rf.c_str());
				rf = exec("/opt/anv-cli/adb shell cat /sdcard/Android/data/org.anvlabs.anvpy/files/console.txt");
				printf("\033[32m%s\033[0m\n", rf.c_str());
			}
			else{ 
				cout << "Invalid project" << endl;
			}
		}
		else{
			cout << "Nothing" << endl;
		}
	}
}
