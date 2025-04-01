#include <iostream>
#include <fstream>
#include <string>

using namespace std;

void updateTestatorStatus() {
    string testatorAadhar;
    cout << "Enter Testator's Aadhaar Number: ";
    cin >> testatorAadhar;
    string statusFile = "../data/" + testatorAadhar + "/status.txt";  // Move up one directory


    // Check if the status file exists
    ifstream checkFile(statusFile);
    if (!checkFile) {
        cout << "[ERROR] Status file not found for the testator!\n";
        return;
    }
    checkFile.close();

    // Update status to DECEASED
    ofstream statusStream(statusFile);
    if (statusStream) {
        statusStream << "DECEASED";
        statusStream.close();
        cout << "[SUCCESS] Testator with Aadhaar " << testatorAadhar << " is now marked as DECEASED.\n";
    } else {
        cout << "[ERROR] Failed to update testator status!\n";
    }
}

int main() {
    cout << "======= Hospital System =======\n";
    updateTestatorStatus();
    return 0;
}
