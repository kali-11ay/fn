#include "DirectOverlay.h"
#include <sstream>
#include <string>
#include <algorithm>
#include <list>
#include <strsafe.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "D3XD/d3dx9math.h"

#define M_PI 3.14159265358979323846264338327950288419716939937510

#define OFFSET_UWORLD 0x9352870
#define botactorid 9284989

float aimfov = 15.0f;

// CHEETOS LIST
bool Menu = true;
bool Aimbot = true;
bool DebugCrap = false;
bool EnemyESP = true;
bool skeleton = true;
bool BoxESP = false;
bool LineESP = true;
//bool ItemESP = true;
bool DistanceESP = true;
bool CircleFOV = true;
bool aimpred = false;
bool actordebug = false;

DWORD processID;
HWND hwnd = NULL;

int width;
int height;
int localplayerID;
float FovAngle;

HANDLE Driver_File;
uint64_t base_address;

DWORD_PTR Uworld;
DWORD_PTR LocalPawn;
DWORD_PTR Localplayer;
DWORD_PTR Rootcomp;
DWORD_PTR PlayerController;
DWORD_PTR Ulevel;

Vector3 localactorpos;

bool isaimbotting;
DWORD_PTR entityx;

FTransform GetBoneIndex(DWORD_PTR mesh, int index)
{
	DWORD_PTR bonearray;
	bonearray = read<DWORD_PTR>(Driver_File, processID, mesh + 0x4A8);
	if (bonearray == NULL)
		bonearray = read<DWORD_PTR>(Driver_File, processID, mesh + 0x4A8 + 0x10);

	return read<FTransform>(Driver_File, processID, bonearray + (index * 0x30));

}
Vector3 GetBoneWithRotation(DWORD_PTR mesh, int id)
{
	FTransform bone = GetBoneIndex(mesh, id);
	FTransform ComponentToWorld = read<FTransform>(Driver_File, processID, mesh + 0x1C0);
	D3DMATRIX Matrix;

	Matrix = MatrixMultiplication(bone.ToMatrixWithScale(), ComponentToWorld.ToMatrixWithScale());

	return Vector3(Matrix._41, Matrix._42, Matrix._43);
}

D3DXMATRIX Matrix(Vector3 rot, Vector3 origin = Vector3(0, 0, 0))
{
	float radPitch = (rot.x * float(M_PI) / 180.f);
	float radYaw = (rot.y * float(M_PI) / 180.f);
	float radRoll = (rot.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}
Vector3 ProjectWorldToScreen(Vector3 WorldLocation)
{
	Vector3 Screenlocation = Vector3(0, 0, 0);
	Vector3 Camera;

	auto chain69 = read<uintptr_t>(Driver_File, processID, Localplayer + 0xa8);
	uint64_t chain699 = read<uintptr_t>(Driver_File, processID, chain69 + 8);

	Camera.x = read<float>(Driver_File, processID, chain699 + 0x7F8);
	Camera.y = read<float>(Driver_File, processID, Rootcomp + 0x12C);

	float test = asin(Camera.x);
	float degrees = test * (180.0 / M_PI);
	Camera.x = degrees;

	if (Camera.y < 0)
		Camera.y = 360 + Camera.y;

	D3DMATRIX tempMatrix = Matrix(Camera);
	Vector3 vAxisX, vAxisY, vAxisZ;

	vAxisX = Vector3(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
	vAxisY = Vector3(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
	vAxisZ = Vector3(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

	uint64_t chain = read<uint64_t>(Driver_File, processID, Localplayer + 0x70);
	uint64_t chain1 = read<uint64_t>(Driver_File, processID, chain + 0x98);
	uint64_t chain2 = read<uint64_t>(Driver_File, processID, chain1 + 0x130);

	Vector3 vDelta = WorldLocation - read<Vector3>(Driver_File, processID, chain2 + 0x10); //camera location credits for Object9999
	Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	float zoom = read<float>(Driver_File, processID, chain699 + 0x590);

	float FovAngle = 80.0f / (zoom / 1.19f);

	float ScreenCenterX = width / 2.0f;
	float ScreenCenterY = height / 2.0f;

	Screenlocation.x = ScreenCenterX + vTransformed.x * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;
	Screenlocation.y = ScreenCenterY - vTransformed.y * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;

	return Screenlocation;
}

std::string builddate = __DATE__;
std::string buildtime = __TIME__;

void menu()
{
	if (Menu)
	{

		// Draw the fancy stuff OwO
		DrawBox(5.f, 5.f, 200.f, 300.f, 0.f, 0.f, 0.f, 0.f, 0.7f, true);
		DrawBox(3, 3, 204, 302, 2.0f, 255, 255, 255, 100, false);
		DrawLine(3, 40, 206, 40, 2.0f, 255, 255, 255, 100);

		DrawString(_xor_("Build Date: ").c_str() + builddate + _xor_(" ").c_str() + buildtime, 20, 10, 8, 255.f, 255.f, 255.f, 255.f);

		if (EnemyESP)
			DrawString(_xor_("Enabled").c_str(), 13, 10 + 150, 10 + 40, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("Disabled").c_str(), 13, 10 + 150, 10 + 40, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("(F1) ESP").c_str(), 13, 10, 10 + 40, 255.f, 255.f, 255.f, 255.f);

		if (BoxESP)
			DrawString(_xor_("Enabled").c_str(), 13, 10 + 150, 10 + 60, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("Disabled").c_str(), 13, 10 + 150, 10 + 60, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("(F2) Box ESP").c_str(), 13, 10, 10 + 60, 255.f, 255.f, 255.f, 255.f);

		if (LineESP)
			DrawString(_xor_("Enabled").c_str(), 13, 10 + 150, 10 + 80, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("Disabled").c_str(), 13, 10 + 150, 10 + 80, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("(F3) Line ESP").c_str(), 13, 10, 10 + 80, 255.f, 255.f, 255.f, 255.f);


		if (DistanceESP)
			DrawString(_xor_("Enabled").c_str(), 13, 10 + 150, 10 + 100, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("Disabled").c_str(), 13, 10 + 150, 10 + 100, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("(F4) Distance ESP").c_str(), 13, 10, 10 + 100, 255.f, 255.f, 255.f, 255.f);

		if (skeleton)
			DrawString(_xor_("Enabled").c_str(), 13, 10 + 150, 10 + 120, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("Disabled").c_str(), 13, 10 + 150, 10 + 120, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("(F5) Bones ESP").c_str(), 13, 10, 10 + 120, 255.f, 255.f, 255.f, 255.f);

		if (Aimbot)
			DrawString(_xor_("Enabled").c_str(), 13, 10 + 150, 10 + 140, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("Disabled").c_str(), 13, 10 + 150, 10 + 140, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("(F6) Aimbot").c_str(), 13, 10, 10 + 140, 255.f, 255.f, 255.f, 255.f);

		if (CircleFOV)
			DrawString(_xor_("Enabled").c_str(), 13, 10 + 150, 10 + 160, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("Disabled").c_str(), 13, 10 + 150, 10 + 160, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("(F7) Circle FOV").c_str(), 13, 10, 10 + 160, 255.f, 255.f, 255.f, 255.f);

		DrawString(_xor_("(Up/Down) FOV Slider").c_str(), 13, 10, 10 + 220, 255.f, 255.f, 255.f, 255.f);
		DrawString((std::to_string(aimfov)).c_str(), 13, 10 + 150, 10 + 220, 255.f, 255.f, 255.f, 255.f);

	}
}

DWORD Menuthread(LPVOID in)
{
	while (1)
	{
		if (GetAsyncKeyState(VK_INSERT) & 1)
			Menu = !Menu;

		if (Menu)
		{
			if (GetAsyncKeyState(VK_F1) & 1)
				EnemyESP = !EnemyESP;

			if (GetAsyncKeyState(VK_F2) & 1)
				BoxESP = !BoxESP;

			if (GetAsyncKeyState(VK_F3) & 1)
				LineESP = !LineESP;

			if (GetAsyncKeyState(VK_F4) & 1)
				DistanceESP = !DistanceESP;

			if (GetAsyncKeyState(VK_F5) & 1)
				skeleton = !skeleton;

			if (GetAsyncKeyState(VK_F6) & 1)
				Aimbot = !Aimbot;

			if (GetAsyncKeyState(VK_F7) & 1)
				CircleFOV = !CircleFOV;

			if (GetAsyncKeyState(VK_F8) & 1)
				aimpred = !aimpred;

			if (GetAsyncKeyState(VK_F9) & 1)
				actordebug = !actordebug;

			if (aimfov) {
				if (GetAsyncKeyState(VK_UP) & 1)
					aimfov += 1;
				if (GetAsyncKeyState(VK_DOWN) & 1)
					aimfov -= 1;
				if (aimfov < 1)
					aimfov = 1;
				if (aimfov > 360)
					aimfov = 360;
			}
		}
	}
}

void DrawSkeleton(DWORD_PTR mesh)
{
	std::list<int> upper_part = { 65,66 };
	std::list<int> right_arm = { 65, BONE_R_ARM_TOP, BONE_R_ARM_LOWER, BONE_MISC_R_HAND_1 };
	std::list<int> left_arm = { 65, BONE_L_ARM_TOP, BONE_L_ARM_LOWER, BONE_MISC_L_HAND };
	std::list<int> spine = { 65, BONE_PELVIS_1 };
	std::list<int> lower_right = { BONE_PELVIS_2, BONE_R_THIGH ,76 };
	std::list<int> lower_left = { BONE_PELVIS_2, BONE_L_THIGH ,69 };
	std::list<std::list<int>> Skeleton = { upper_part, right_arm, left_arm, spine, lower_right, lower_left };

	if (!mesh) return;

	Vector3 neckpos = GetBoneWithRotation(mesh, 65);
	Vector3 pelvispos = GetBoneWithRotation(mesh, BONE_PELVIS_2);

	Vector3 previous(0, 0, 0);
	Vector3 current, p1, c1;

	for (auto a : Skeleton)
	{
		previous = Vector3(0, 0, 0);
		for (int bone : a)
		{
			current = bone == 65 ? neckpos : (bone == BONE_PELVIS_2 ? pelvispos : GetBoneWithRotation(mesh, bone));
			if (previous.x == 0.f)
			{
				previous = current;
				continue;
			}

			p1 = ProjectWorldToScreen(previous);
			c1 = ProjectWorldToScreen(current);


			if (p1.x != 0 && p1.y != 0 && c1.x != 0 && c1.y != 0)
				DrawLine(p1.x, p1.y, c1.x, c1.y, (float)2.5, 255.f, 255.f, 255.f, 200.f);

			previous = current;
		}
	}
}

void DrawCorneredBox(int X, int Y, int W, int H) {

	float lineW = (W / 4.5);
	float lineH = (H / 4.5);

	DrawLine(X, Y, X, Y + lineH, (float)2.5, 255.f, 255.f, 255.f, 200.f);
	DrawLine(X, Y, X + lineW, Y, (float)2.5, 255.f, 255.f, 255.f, 200.f);
	DrawLine(X + W - lineW, Y, X + W, Y, (float)2.5, 255.f, 255.f, 255.f, 200.f);
	DrawLine(X + W, Y, X + W, Y + lineH, (float)2.5, 255.f, 255.f, 255.f, 200.f);
	DrawLine(X, Y + H - lineH, X, Y + H, (float)2.5, 255.f, 255.f, 255.f, 200.f);
	DrawLine(X, Y + H, X + lineW, Y + H, (float)2.5, 255.f, 255.f, 255.f, 200.f);
	DrawLine(X + W - lineW, Y + H, X + W, Y + H, (float)2.5, 255.f, 255.f, 255.f, 200.f);
	DrawLine(X + W, Y + H - lineH, X + W, Y + H, (float)2.5, 255.f, 255.f, 255.f, 200.f);
}

void drawLoop(int width, int height) {
	menu();

	float radiusx = aimfov * (width / 2 / 100);
	float radiusy = aimfov * (height / 2 / 100);

	float calcradius = (radiusx + radiusy) / 2;

	DrawCircle(width / 2, height / 2, calcradius, 2.f, 0.f, 255.f, 0.f, 255.f, false);

	Uworld = read<DWORD_PTR>(Driver_File, processID, base_address + OFFSET_UWORLD);
	//printf(_xor_("Uworld: %p.\n").c_str(), Uworld);

	DWORD_PTR Gameinstance = read<DWORD_PTR>(Driver_File, processID, Uworld + 0x180);

	if (Gameinstance == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Gameinstance: %p.\n").c_str(), Gameinstance);

	DWORD_PTR LocalPlayers = read<DWORD_PTR>(Driver_File, processID, Gameinstance + 0x38);

	if (LocalPlayers == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("LocalPlayers: %p.\n").c_str(), LocalPlayers);

	Localplayer = read<DWORD_PTR>(Driver_File, processID, LocalPlayers);

	if (Localplayer == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("LocalPlayer: %p.\n").c_str(), Localplayer);

	PlayerController = read<DWORD_PTR>(Driver_File, processID, Localplayer + 0x30);

	if (PlayerController == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("playercontroller: %p.\n").c_str(), PlayerController);

	LocalPawn = read<uint64_t>(Driver_File, processID, PlayerController + 0x2A0);

	if (LocalPawn == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Pawn: %p.\n").c_str(), LocalPawn);

	Rootcomp = read<uint64_t>(Driver_File, processID, LocalPawn + 0x130);

	if (Rootcomp == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Rootcomp: %p.\n").c_str(), Rootcomp);

	if (LocalPawn != 0) {
		localplayerID = read<int>(Driver_File, processID, LocalPawn + 0x18);
	}

	Ulevel = read<DWORD_PTR>(Driver_File, processID, Uworld + 0x30);
	//printf(_xor_("Ulevel: %p.\n").c_str(), Ulevel);

	if (Ulevel == (DWORD_PTR)nullptr)
		return;

	DWORD64 PlayerState = read<DWORD64>(Driver_File, processID, LocalPawn + 0x240);

	if (PlayerState == (DWORD_PTR)nullptr)
		return;

	DWORD ActorCount = read<DWORD>(Driver_File, processID, Ulevel + 0xA0);

	DWORD_PTR AActors = read<DWORD_PTR>(Driver_File, processID, Ulevel + 0x98);
	//printf(_xor_("AActors: %p.\n").c_str(), AActors);

	if (AActors == (DWORD_PTR)nullptr)
		return;

	for (int i = 0; i < ActorCount; i++)
	{
		uint64_t CurrentActor = read<uint64_t>(Driver_File, processID, AActors + i * 0x8);

		int curactorid = read<int>(Driver_File, processID, CurrentActor + 0x18);

		if (curactorid == localplayerID || curactorid == 18388801)
		{
			if (CurrentActor == (uint64_t)nullptr || CurrentActor == -1 || CurrentActor == NULL)
				continue;

			uint64_t CurrentActorRootComponent = read<uint64_t>(Driver_File, processID, CurrentActor + 0x130);

			if (CurrentActorRootComponent == (uint64_t)nullptr || CurrentActorRootComponent == -1 || CurrentActorRootComponent == NULL)
				continue;

			uint64_t currentactormesh = read<uint64_t>(Driver_File, processID, CurrentActor + 0x280);

			if (currentactormesh == (uint64_t)nullptr || currentactormesh == -1 || currentactormesh == NULL)
				continue;

			//int MyTeamId = read<int>(Driver_File, processID, PlayerState + 0xED0);

			DWORD64 otherPlayerState = read<uint64_t>(Driver_File, processID, CurrentActor + 0x240);

			if (otherPlayerState == (uint64_t)nullptr || otherPlayerState == -1 || otherPlayerState == NULL)
				continue;

			//int ActorTeamId = read<int>(Driver_File, processID, otherPlayerState + 0xED0);

			Vector3 Headpos = GetBoneWithRotation(currentactormesh, 66);

			localactorpos = read<Vector3>(Driver_File, processID, Rootcomp + 0x11C);

			float distance = localactorpos.Distance(Headpos) / 100.f;

			if (distance < 1.5f)
				continue;

			//W2S
			Vector3 HeadposW2s = ProjectWorldToScreen(Headpos);
			Vector3 bone0 = GetBoneWithRotation(currentactormesh, 0);
			Vector3 bottom = ProjectWorldToScreen(bone0);
			Vector3 Headbox = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 15));
			Vector3 Aimpos = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 10));

			float Height1 = abs(Headbox.y - bottom.y);
			float Width1 = Height1 * 0.65;

			if (skeleton)
				DrawSkeleton(currentactormesh);

			if (BoxESP) {
				float Height = Headbox.y - bottom.y;
				if (Height < 0)
					Height = Height * (-1.f);
				float Width = Height * 0.65;
				Headbox.x = Headbox.x - (Width / 2);
				//DrawBox(Headbox.x - (Width1 / 2), Headbox.y, Width1, Height1, 2.f, 255.f, 0.f, 0.f, 200.f, false);
				DrawCorneredBox(Headbox.x, Headbox.y, Width, Height);
			}

			if (EnemyESP)
				DrawString(_xor_("Enemy").c_str(), 13, HeadposW2s.x, HeadposW2s.y - 25, 0, 1, 1);

			if (DistanceESP)
			{
				CHAR dist[50];
				sprintf_s(dist, _xor_("[%.f]").c_str(), distance);

				DrawString(dist, 13, HeadposW2s.x + 40, HeadposW2s.y - 25, 0, 1, 1);
			}

			if (LineESP)
				DrawLine(width / 2, height, bottom.x, bottom.y, 2.f, 255.f, 0.f, 0.f, 200.f);


			//if (MyTeamId == ActorTeamId) continue;

			if (Aimbot)
			{
				float headX = HeadposW2s.x - width / 2;
				float headY = HeadposW2s.y - height / 2;

				if (headX >= -100 && headX <= 100 && headY >= -100 && headY <= 100 && distance < 350) {

					if (GetAsyncKeyState(VK_RBUTTON)) {
						mouse_event(MOUSEEVENTF_MOVE, headX, headY, NULL, NULL);
					}
				}
			}
		}
	}
}

LPCSTR RandomStringx(int len)
{
	std::string str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::string newstr;
	int pos;
	while (newstr.size() != len) {
		pos = ((rand() % (str.size() - 1)));
		newstr += str.substr(pos, 1);
	}
	return newstr.c_str();
}


void main()
{
	SetConsoleTitle(RandomStringx(32));
	DWORD dLastError = GetLastError();
	LPCTSTR strErrorMessage = NULL;

	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		dLastError,
		0,
		(LPSTR)&strErrorMessage,
		0,
		NULL);

	Driver_File = CreateFileW(_xor_(L"\\\\.\\doesshelikemetho").c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	while (Driver_File == INVALID_HANDLE_VALUE)
	{
		Driver_File = CreateFileW(_xor_(L"\\\\.\\doesshelikemetho").c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		printf(_xor_("Load the driver.\nError: ").c_str());
		printf(strErrorMessage);
		Sleep(1000);
		system("CLS");
	}

	while (hwnd == NULL)
	{
		hwnd = FindWindowA(0, _xor_("Fortnite  ").c_str());
		printf(_xor_("Load Fortnite.\n").c_str());
		Sleep(1000);
		system("CLS");
	}
	GetWindowThreadProcessId(hwnd, &processID);

	RECT rect;
	if (GetWindowRect(hwnd, &rect))
	{
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	info_t Input_Output_Data;
	Input_Output_Data.pid = processID;
	unsigned long int Readed_Bytes_Amount;

	DeviceIoControl(Driver_File, ctl_base, &Input_Output_Data, sizeof Input_Output_Data, &Input_Output_Data, sizeof Input_Output_Data, &Readed_Bytes_Amount, nullptr);
	base_address = (unsigned long long int)Input_Output_Data.data;

	system("CLS");
	std::printf(_xor_("Base addy is %p.\n").c_str(), (void*)base_address);

	CreateThread(NULL, NULL, Menuthread, NULL, NULL, NULL);

	DirectOverlaySetOption(D2DOV_DRAW_FPS | D2DOV_FONT_COURIER);
	DirectOverlaySetup(drawLoop, hwnd);

	getchar();
}