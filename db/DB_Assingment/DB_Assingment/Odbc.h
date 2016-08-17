
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <clocale>
#include <codecvt>
#include "sql.h"
#include "sqlext.h"
#include <string>
#include <vector>


class Odbc
{
public:
	static Odbc* _Inst;
	static Odbc* Odbc::GetInstance();

	bool			IsConnect(){ return _IsConnect; }

	bool			Connect(wchar_t* odbcName, wchar_t* mysqlId, wchar_t* password);
	void			Disonnect();

	int				GetUserId(std::string userName);
	std::string		GetPassword(std::string name);

	bool			InsertData(std::string tableName, std::string colNames, std::string value);
	bool			DeleteData(std::string tableName, std::wstring id, std::wstring pw);
	bool			CheckDataExist(std::string tableName, std::string colName, std::string value);
	bool			CheckDataExist(std::string tableName, std::string colName, std::wstring value);
	std::string		SelectData(std::string tableName, std::string colNames, bool whereCon = false, std::string whereCol = "id", std::string value = "");
	bool			PushQuery(std::wstring query);
	
	void			ReadFileAndInsert(const char *path);
	void			CheckSuccess(int ret);

	bool			AddUser(std::wstring id, std::wstring password);
	bool			CheckLogin(std::wstring id, std::wstring password);
	bool			DeleteUser(std::wstring id, std::wstring password);

	bool isUserExist(std::wstring id, std::wstring pw);

	
	const char*		CreateCSV(const char* filename, int num);
	void			SelectBookData();
	
	std::wstring	utf8_to_wstring(const std::string& str);
	std::string		wstring_to_utf8(const std::wstring& str);

private:
	Odbc();
	~Odbc();

	bool		_IsConnect = false;
	SQLHENV		_hEnv;
	SQLHDBC		_hDbc;
	SQLHSTMT	_hStmt;
};