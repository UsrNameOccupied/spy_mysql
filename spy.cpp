#include <iostream>
#include <string>
#include <mysql/mysql.h>
#include <cstdlib>
#include <sstream>
#include <map>

// g++ -o spy spy.cpp `mysql_config --cflags --libs`

using namespace std;

class MyDB {
public:
    MyDB();
    ~MyDB();
    bool initDB(string host, string user, string pwd, string db_name, int db_port);
    bool exeSQL(string sql);
    bool spy_table(string host, string user, string pwd, string db_name, int db_port, const string& table_name, int& key_field_index, map<string, string>& map_field_name_type);
private:
    MYSQL *connection;
    MYSQL_RES *result;
    MYSQL_FIELD *fields;
    MYSQL_ROW row;
};

MyDB::MyDB() {
    // 初始化数据库连接变量
    connection = mysql_init(NULL);

    if(connection == NULL) {
        cout << "Error:" << mysql_error(connection);
        exit(1);
    }
}

MyDB::~MyDB() {
    // 关闭数据库连接
    if(connection != NULL) {
        mysql_close(connection);
    }
}

bool MyDB::initDB(string host, string user, string pwd, string db_name, int db_port) {
    // 函数mysql_real_connect建立一个数据库连接
    // 成功返回MYSQL*连接句柄，失败返回NULL
    connection = mysql_real_connect(connection, host.c_str(),
                                    user.c_str(), pwd.c_str(), db_name.c_str(), db_port, NULL, 0);

    if(connection == NULL) {
        cout << "Error:" << mysql_error(connection);
        exit(1);
    }

    return true;
}

string type_convert(int mysql_field_type) {
    string type;

    switch (mysql_field_type) {
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_LONGLONG:
    case MYSQL_TYPE_INT24:
        type = "uint64_t";
        break;

    case MYSQL_TYPE_FLOAT:
    case MYSQL_TYPE_DOUBLE:
        type = "double";
        break;

    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_TIME:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_YEAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
        type = "string";
        break;

    default:
        type = "string";
        break;
    }

    return type;
}

string type_convert(const string& mysql_field_type) {
    string type;

    if(mysql_field_type.find("int") != string::npos) {
        type = "uint64_t";
    } else if(mysql_field_type.find("text") != string::npos) {
        type = "string";
    } else if(mysql_field_type.find("char") != string::npos) {
        type = "string";
    } else if(mysql_field_type.find("date") != string::npos) {
        type = "string";
    } else if(mysql_field_type.find("time") != string::npos) {
        type = "string";
    } else if(mysql_field_type.find("year") != string::npos) {
        type = "string";
    } else if(mysql_field_type.find("float") != string::npos) {
        type = "double";
    } else if(mysql_field_type.find("double") != string::npos) {
        type = "double";
    } else {
        type = "string";
    }

    return type;
}

string default_value_of_type(const string& mysql_field_type) {
    string val;

    if(mysql_field_type.find("int") != string::npos) {
        val = "0";
    } else if(mysql_field_type.find("text") != string::npos) {
        val = "\"\"";
    } else if(mysql_field_type.find("char") != string::npos) {
        val = "\"\"";
    } else if(mysql_field_type.find("date") != string::npos) {
        val = "\"\"";
    } else if(mysql_field_type.find("time") != string::npos) {
        val = "\"\"";
    } else if(mysql_field_type.find("year") != string::npos) {
        val = "\"\"";
    } else if(mysql_field_type.find("float") != string::npos) {
        val = "0";
    } else if(mysql_field_type.find("double") != string::npos) {
        val = "0";
    } else {
        val = "\"\"";
    }

    return val;
}

void generate_header() {
    ostringstream oss;
    
    oss << "#include <iostream>\n"
        << "#include <string>\n"
        << "#include <mysql/mysql.h>\n"
        << "#include <cstdlib>\n"
        << "#include <sstream>\n"
        << "#include <vector>\n"
        << "#include <map>\n"
        << "#include <inttypes.h>\n\n"
        << "using namespace std;\n\n"
        << "MYSQL *connection;\n"
        << "MYSQL_RES *result;\n"
        << "MYSQL_ROW row;\n\n"
        << "void init_mysql(string host, string user, string pwd, string db_name, int db_port) {\n"
        << "  // 初始化数据库连接变量\n  connection = mysql_init(NULL);"
        << "\n"
        << "  if(connection == NULL) {\n"
        << "    cout << \"error:\" << mysql_error(connection);\n"
        << "    exit(1);\n"
        << "  }\n"
        << "  // 函数mysql_real_connect建立一个数据库连接\n"
        << "  // 成功返回MYSQL*连接句柄，失败返回NULL\n"
        << "  connection = mysql_real_connect(connection, host.c_str(), user.c_str(), pwd.c_str(), db_name.c_str(), db_port, NULL, 0);\n"
        << " \n"
        << "  if(connection == NULL) {\n"
        << "    cout << \"error:\" << mysql_error(connection);\n"
        << "    exit(1);\n"
        << "  }\n"
        << "  if (!mysql_set_character_set(connection, \"utf8\")) {\n"
        << "    cout << \"New client character set: \" << mysql_character_set_name(connection) << endl;\n"
        << "  }\n"
        << "}\n\n";
        
    cout << oss.str() << endl;
}

void generate_main(string host, string user, string pwd, string db_name, int db_port, const string& table_names) {
    ostringstream oss;
    
    oss << "int main() {\n"
        << "init_mysql("
        << "\"" << host << "\""
        << ","
        << "\"" << user << "\"" 
        << ","
        << "\"" << pwd << "\"" 
        << ","
        << "\"" << db_name << "\"" 
        << ","
        << db_port
        << ");\n"
        << "vector<"
        << table_names
        << "> vec_data_set;\n"
        << "select_all(vec_data_set);\n\n"
        << "}\n\n";
        
    cout << oss.str() << endl;
}

void generate_beans(const string& table_names, int key_field_index, map<string, string>& map_field_name_type) {
    ostringstream oss;
    oss << "class "
        << table_names
        << " {\n";

    map<string, string>::iterator map_field_name_type_it;

    for (map_field_name_type_it = map_field_name_type.begin(); map_field_name_type_it != map_field_name_type.end(); map_field_name_type_it++) {
        string type = type_convert(map_field_name_type_it->second);

        oss << "  " << type
            << " "
            << map_field_name_type_it->first
            << ";\n";

    }

    // 标记字段的值是否被设置
    oss << "  // 标记字段的值是否被设置\n"
        << "  map<int, int> field_set_flag;\n";

    // 记录主键的字段位置
    oss << "  // 记录主键的字段位置\n"
        << "  int key_field_index;\n";

    oss << "public:\n";
    oss << "  " << table_names << "() {\n";

    for (map_field_name_type_it = map_field_name_type.begin(); map_field_name_type_it != map_field_name_type.end(); map_field_name_type_it++) {
        string val = default_value_of_type(map_field_name_type_it->second);

        oss << "    " << map_field_name_type_it->first
            << " = "
            << val
            << ";\n";
    }

    oss << "    key_field_index = " << key_field_index
        << ";\n";

    oss << "  }\n";
    
    oss << "  void to_string() {\n";
    oss << "    ostringstream oss;\n";
    for (map_field_name_type_it = map_field_name_type.begin(); map_field_name_type_it != map_field_name_type.end(); map_field_name_type_it++) {
        oss << "    oss << \"" << map_field_name_type_it->first << "\" << \"=\" << " << map_field_name_type_it->first << " << \";\";\n";
    }
    oss << "    cout << oss.str() << endl;\n";
    oss << "  }\n";

    int field_index = 0;

    for (map_field_name_type_it = map_field_name_type.begin(); map_field_name_type_it != map_field_name_type.end(); map_field_name_type_it++) {
        string type = type_convert(map_field_name_type_it->second);

        oss << "  void set_"
            << map_field_name_type_it->first
            << "("
            << type
            << " "
            << map_field_name_type_it->first
            << ") {\n"
            << "    this->"
            << map_field_name_type_it->first
            << " = "
            << map_field_name_type_it->first
            << ";\n"
            << "    field_set_flag["
            << field_index
            << "] = 1;\n"
            << "  }\n\n";

        oss << "  " << type
            << " get_"
            << map_field_name_type_it->first
            << "() const {\n"
            << "    return this->"
            << map_field_name_type_it->first
            << ";\n"
            << "  }\n\n";

        field_index++;
    }

    oss << "  int get_key_field_index() const {\n"
        << "    return this->key_field_index;\n"
        << "  }\n\n";

    // 获取字段是否被设置
    oss << "  // 获取字段是否被设置\n"
        << "  int get_field_set_flag(int field_id) const {\n"
        << "    return (this->field_set_flag.find(field_id) != this->field_set_flag.end()) ? 1 : 0;\n"
        << "  }\n\n";

    oss << "};";

    cout << oss.str() << endl;
}

void generate_select_all_code(const string& table_names, map<string, string>& map_field_name_type) {
    ostringstream oss;
    map<string, string>::iterator map_field_name_type_it;

    oss << "\nvoid select_all( vector<"
        << table_names
        << ">& vec_data_set) {\n";

    oss << "  string sql = \"select * from " + table_names + "\";\n";
    oss << "  cout << sql << endl;\n";

    oss << "\n  if(mysql_query(connection, sql.c_str())) {\n"
        "    cout << \"error:\" << mysql_error(connection) << endl;\n"
        "  } else {\n"
        "    // 获取结果集\n"
        "    result = mysql_use_result(connection);\n"
        "     \n"
        "    if(result) {\n"
        "    // 获取下一行\n"
        "      while((row = mysql_fetch_row(result))) {\n";

    oss << "        " << table_names
        << " item;\n";

    int field_index = 0;

    for (map_field_name_type_it = map_field_name_type.begin(); map_field_name_type_it != map_field_name_type.end(); map_field_name_type_it++) {
        string type = type_convert(map_field_name_type_it->second);

        if(type.compare("string") == 0) {
            oss << "        item.set_"
                << map_field_name_type_it->first
                << "(row[" <<field_index << "]);\n";
        } else if (type.compare("uint64_t") == 0) {
            oss << "        item.set_"
                << map_field_name_type_it->first
                << "(atoll(row[" <<field_index << "]));\n";
        } else if (type.compare("double") == 0) {
            oss << "        item.set_"
                << map_field_name_type_it->first
                << "(atof(row[" <<field_index << "]));\n";
        }

        field_index++;
    }

    oss << "        item.to_string();\n        vec_data_set.push_back("
        << "item);\n";

    oss << "      }\n"
        << "    }\n"
        << "  }\n"
        << "}\n";

    cout << oss.str() << endl;
}

void generate_select_code(const string& table_names, map<string, string>& map_field_name_type) {
    ostringstream oss;
    map<string, string>::iterator map_field_name_type_it;

    oss << "\nvoid select( const "
        << table_names
        << "& cond, "
        << "vector<"
        << table_names
        << ">& vec_data_set) {\n";

    oss << "  ostringstream sql;\n  sql << \"select * from " + table_names + " where \";\n";

    int field_index = 0;

    for (map_field_name_type_it = map_field_name_type.begin(); map_field_name_type_it != map_field_name_type.end(); map_field_name_type_it++) {
        string type = type_convert(map_field_name_type_it->second);

        if(type.compare("string") == 0) {
            oss << "  if(cond.get_field_set_flag(" << field_index << ")) {\n"
                << "    sql << \"" << map_field_name_type_it->first << "=\""
                << "    << \"\\\"\" << cond.get_" << map_field_name_type_it->first << "() << \"\\\"\" << \" and \";\n"
                << "  }\n";

        } else {

            oss << "  if(cond.get_field_set_flag(" << field_index << ")) {\n"
                << "    sql << \"" << map_field_name_type_it->first << "=\""
                << "    << cond.get_" << map_field_name_type_it->first << "() << \" and \";\n"
                << "  }\n";
        }

        field_index++;
    }

    oss << "\n  string tmp_content = sql.str();\n"
        << "  size_t index = tmp_content.find_last_of(\"<< \\\" and \\\"\");\n"
        << "  tmp_content = (index !=  string::npos) ? tmp_content.substr(0, index) : tmp_content;\n"
        << "  sql.str(\"\");\n"
        << "  sql << tmp_content << \" \";\n\n";

    oss << "  cout << sql.str() << endl;\n";
        
    oss << "  if(mysql_query(connection, sql.str().c_str())) {\n"
        "    cout << \"error:\" << mysql_error(connection) << endl;\n"
        "  } else {\n"
        "    // 获取结果集\n"
        "    result = mysql_use_result(connection);\n"
        "     \n"
        "    if(result) {\n"
        "      // 获取下一行\n"
        "      while((row = mysql_fetch_row(result))) {\n";

    oss << "        " << table_names
        << " item;\n";

    field_index = 0;

    for (map_field_name_type_it = map_field_name_type.begin(); map_field_name_type_it != map_field_name_type.end(); map_field_name_type_it++) {
        string type = type_convert(map_field_name_type_it->second);

        if(type.compare("string") == 0) {
            oss << "        item.set_"
                << map_field_name_type_it->first
                << "(row[" <<field_index << "]);\n";
        } else if (type.compare("uint64_t") == 0) {
            oss << "        item.set_"
                << map_field_name_type_it->first
                << "(atoll(row[" <<field_index << "]));\n";
        } else if (type.compare("double") == 0) {
            oss << "        item.set_"
                << map_field_name_type_it->first
                << "(atof(row[" <<field_index << "]));\n";
        }

        field_index++;
    }

    oss << "        item.to_string();\n        vec_data_set.push_back("
        << "item);\n";

    oss << "      }\n"
        << "    }\n"
        << "  }\n"
        << "}\n";

    cout << oss.str() << endl;
}

void generate_update_code(const string& table_names, int key_field_index, map<string, string>& map_field_name_type) {
    ostringstream oss;
    map<string, string>::iterator map_field_name_type_it;

    oss << "\nvoid update( const "
        << table_names
        << "& cond) {\n";

    oss << "  ostringstream sql;\n  sql << \"update " + table_names + " set \";\n";

    int field_index = 0;

    for (map_field_name_type_it = map_field_name_type.begin(); map_field_name_type_it != map_field_name_type.end(); map_field_name_type_it++) {
        string type = type_convert(map_field_name_type_it->second);

        if(field_index != key_field_index) {
            if(type.compare("string") == 0) {
                oss << "  if(cond.get_field_set_flag(" << field_index << ")) {\n"
                    << "    sql << \"" << map_field_name_type_it->first << "=\""
                    << "    << \"\\\"\" << cond.get_" << map_field_name_type_it->first << "() << \"\\\"\" << \" and \";\n"
                    << "  }\n";

            } else {
                oss << "  if(cond.get_field_set_flag(" << field_index << ")) {\n"
                    << "    sql << \"" << map_field_name_type_it->first << "=\""
                    << "    << cond.get_" << map_field_name_type_it->first << "() << \" and \";\n"
                    << "  }\n";
            }
        }

        field_index++;
    }

    oss << "\n  string tmp_content = sql.str();\n"
        << "  size_t index = tmp_content.find_last_of(\"<< \\\" and \\\"\");\n"
        << "  tmp_content = (index !=  string::npos) ? tmp_content.substr(0, index) : tmp_content;\n"
        << "  sql.str(\"\");\n"
        << "  sql << tmp_content << \" \";\n\n";

    field_index = 0;

    for (map_field_name_type_it = map_field_name_type.begin(); map_field_name_type_it != map_field_name_type.end(); map_field_name_type_it++) {
        string type = type_convert(map_field_name_type_it->second);

        if(field_index == key_field_index) {
            if(type.compare("string") == 0) {
                oss << "  sql << \"where \" << \"" << map_field_name_type_it->first << "=\""
                    << "    << \"\\\"\" << cond.get_" << map_field_name_type_it->first << "() << \"\\\"\";\n";
            } else {
                oss << "  sql << \"where \" << \"" << map_field_name_type_it->first << "=\""
                    << "    << cond.get_" << map_field_name_type_it->first << "();\n";
            }
        }

        field_index++;
    }

    oss << "  cout << sql.str() << endl;\n";
 
    oss << "\n  if(mysql_query(connection, sql.str().c_str())) {\n"
        "    cout << \"error:\" << mysql_error(connection) << endl;\n"
        "  } else {\n"
        "    cout << \"ok!\" << endl;\n"
        << "  }\n"
        << "}\n";

    cout << oss.str() << endl;
}

void generate_insert_code(const string& table_names, int key_field_index, map<string, string>& map_field_name_type) {
    ostringstream oss;
    map<string, string>::iterator map_field_name_type_it;

    oss << "\nvoid insert( const "
        << table_names
        << "& cond) {\n";

    oss << "  ostringstream sql;\n  sql << \"insert into " + table_names + " ( \" ;\n";

    int field_index = 0;

    for (map_field_name_type_it = map_field_name_type.begin(); map_field_name_type_it != map_field_name_type.end(); map_field_name_type_it++) {
        string type = type_convert(map_field_name_type_it->second);

        oss << "  if(cond.get_field_set_flag(" << field_index << ")) {\n"
            << "    sql << \"`" << map_field_name_type_it->first << "`,\";\n"
            << "  }\n";

        field_index++;
    }

    oss << "\n  string tmp_content = sql.str();\n"
        << "  size_t index = tmp_content.find_last_of(\",\");\n"
        << "  tmp_content = (index !=  string::npos) ? tmp_content.substr(0, index) : tmp_content;\n"
        << "  sql.str(\"\");\n"
        << "  sql << tmp_content << \") values (\";\n\n";

    field_index = 0;

    for (map_field_name_type_it = map_field_name_type.begin(); map_field_name_type_it != map_field_name_type.end(); map_field_name_type_it++) {
        string type = type_convert(map_field_name_type_it->second);

        if(field_index != key_field_index) {
            if(type.compare("string") == 0) {
                oss << "  if(cond.get_field_set_flag(" << field_index << ")) {\n"
                    << "    sql << \"\\\"\" << cond.get_" << map_field_name_type_it->first << "() << \"\\\"\" << \" , \";\n"
                    << "  }\n";

            } else {
                oss << "  if(cond.get_field_set_flag(" << field_index << ")) {\n"
                    << "    sql << cond.get_" << map_field_name_type_it->first << "() << \" , \";\n"  
                    << "  }\n";
            }
        }

        field_index++;
    }
        
    oss << "\n  tmp_content = sql.str();\n"
        << "  index = tmp_content.find_last_of(\",\");\n"
        << "  tmp_content = (index !=  string::npos) ? tmp_content.substr(0, index) : tmp_content;\n"
        << "  sql.str(\"\");\n"
        << "  sql << tmp_content << \")\";\n\n";

    oss << "  cout << sql.str() << endl;\n";
    
    oss << "\n  if(mysql_query(connection, sql.str().c_str())) {\n"
        "    cout << \"error:\" << mysql_error(connection) << endl;\n"
        "  } else {\n"
        "    cout << \"ok!\" << endl;\n"
        << "  }\n"
        << "}\n";

    cout << oss.str() << endl;
}

// 获取表的字段以及类型并生成bean
bool MyDB::spy_table(string host, string user, string pwd, string db_name, int db_port, const string& table_name, int& key_field_index, map<string, string>& map_field_name_type) {
    map_field_name_type.clear();
    string sql = string("desc") + string(" ") + table_name;

    if(mysql_query(connection, sql.c_str())) {
        cout << "error:" << mysql_error(connection) << endl;
    } else {
        // 获取字段数量
        int field_count = mysql_field_count(connection);
        // 获取结果集
        result = mysql_use_result(connection);
        int field_index = 0;

        if(result) {
            // 获取下一行
            while((row = mysql_fetch_row(result))) {
                map_field_name_type[row[0]] = row[1];
                string key_field = row[3];

                if(key_field.compare("PRI") == 0) {
                    key_field_index = field_index;
                }

                field_index++;
            }

            generate_header();
            generate_beans(table_name, key_field_index, map_field_name_type);
            generate_update_code(table_name, key_field_index, map_field_name_type);
            generate_select_all_code(table_name, map_field_name_type);
            generate_select_code(table_name, map_field_name_type);
            generate_insert_code(table_name, key_field_index, map_field_name_type);
            generate_main(host, user, pwd, db_name, db_port, table_name);
        }
    }
}

int main() {
    map<string, string> map_field_name_type;
    int key_index;
    MyDB db;
    db.initDB("localhost", "root", "xx9831XXJ", "library", 3369);
    db.spy_table("localhost", "root", "xx9831XXJ", "library", 3369, "book_info", key_index, map_field_name_type);
    return 0;
}




