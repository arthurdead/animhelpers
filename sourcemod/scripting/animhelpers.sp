#include <sourcemod>
#include <animhelpers>
#include <regex>

#define MATERIAL_MODIFY_STRING_SIZE 255

#define MATERIAL_MODIFY_MODE_NONE 0
#define MATERIAL_MODIFY_MODE_SETVAR 1

public Plugin myinfo = 
{
	name = "animhelpers",
	author = "Arthurdead",
	description = "",
	version = "0.1.0.2",
	url = ""
};

static char vmt_varname[64];
static char vmt_varvalue[PLATFORM_MAX_PATH];

static ArrayList processed_materials;
static ArrayList processed_incmdls;

enum struct MatVarInfo
{
	int control;

	char value[MATERIAL_MODIFY_STRING_SIZE];

	float time;
}

enum struct EntityMatVarInfo
{
	int ref;

	StringMap variables;
}

static StringMap mat_ent_map;

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int length)
{
	RegPluginLibrary("animhelpers");
	CreateNative("AddModelToDownloadsTable", native_AddModelToDownloadsTable);
	CreateNative("SetMaterialVar", native_set_material_var);
	return APLRes_Success;
}

static Regex vmt_cond_regex;

public void OnPluginStart()
{
	mat_ent_map = new StringMap();

	processed_incmdls = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));
	processed_materials = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));

	char err[128];
	RegexError errcode = REGEX_ERROR_NONE;
	vmt_cond_regex = new Regex(
		"(?:[a-z]_)?(?:(?:[<>]=?)?dx(?:9[05]?|6)(?:_20b)?|ldr|srgb|hdr)"
		, 0, err, sizeof(err), errcode
	);
	if(errcode != REGEX_ERROR_NONE) {
		SetFailState("failed to create vmt conditional regex: %s (%i)", err, errcode);
	}

	RegAdminCmd("sm_printdltable", sm_printdltable, ADMFLAG_ROOT);
}

static Action sm_printdltable(int client, int args)
{
	int table = FindStringTable("downloadables");

	int len = GetStringTableNumStrings(table);

	char tmpstr[PLATFORM_MAX_PATH];

	for(int i = 0; i < len; ++i) {
		ReadStringTable(table, i, tmpstr, PLATFORM_MAX_PATH);
		PrintToConsole(client, "%s", tmpstr);
	}

	return Plugin_Handled;
}

static void lowerstr(char[] name)
{
	int i = 0;
	while(name[i] != '\0') {
		name[i] = CharToLower(name[i]);
		++i;
	}
}

static void parse_material_dl(KeyValues mat)
{
	if(mat.GotoFirstSubKey(false)) {
		do {
			mat.GetSectionName(vmt_varname, sizeof(vmt_varname));
			lowerstr(vmt_varname);

			if(StrContains(vmt_varname, "360?") == 0) {
				continue;
			}

			mat.GetString(NULL_STRING, vmt_varvalue, PLATFORM_MAX_PATH);

			if(StrEqual(vmt_varname, "$basetexture") ||
				StrEqual(vmt_varname, "$lightwarptexture") ||
				StrEqual(vmt_varname, "$bumpmap") ||
				StrEqual(vmt_varname, "$phongexponenttexture") ||
				StrEqual(vmt_varname, "$sheenmap") ||
				StrEqual(vmt_varname, "$sheenmapmask") ||
				StrEqual(vmt_varname, "$envmapmask") ||
				StrEqual(vmt_varname, "$detail") ||
				StrEqual(vmt_varname, "$selfillummask")) {
				int ext = StrContains(vmt_varvalue, ".vtf");
				if(ext != -1) {
					vmt_varvalue[ext] = '\0';
				}

				Format(vmt_varvalue, PLATFORM_MAX_PATH, "materials/%s.vtf", vmt_varvalue);
				AddFileToDownloadsTable_fixed(vmt_varvalue);
			} else if(StrEqual(vmt_varname, "$envmap")) {
				if(!StrEqual(vmt_varvalue, "env_cubemap")) {
					int ext = StrContains(vmt_varvalue, ".hdr.vtf");
					if(ext != -1) {
						vmt_varvalue[ext] = '\0';
					} else {
						ext = StrContains(vmt_varvalue, ".vtf");
						if(ext != -1) {
							vmt_varvalue[ext] = '\0';
						}
					}

					char tmpvalue[PLATFORM_MAX_PATH];
					strcopy(tmpvalue, PLATFORM_MAX_PATH, vmt_varvalue);

					FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "materials/%s.vtf", tmpvalue);
					if(FileExists(vmt_varvalue, true)) {
						AddFileToDownloadsTable_fixed(vmt_varvalue);
					}
					FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "materials/%s.hdr.vtf", tmpvalue);
					if(FileExists(vmt_varvalue, true)) {
						AddFileToDownloadsTable_fixed(vmt_varvalue);
					}
				}
			} else if(StrEqual(vmt_varname, "$material") ||
						StrEqual(vmt_varname, "$modelmaterial")) {
				int ext = StrContains(vmt_varvalue, ".vmt");
				if(ext != -1) {
					vmt_varvalue[ext] = '\0';
				}

				Format(vmt_varvalue, PLATFORM_MAX_PATH, "materials/%s.vmt", vmt_varvalue);
				AddFileToDownloadsTable_fixed(vmt_varvalue);
			} else if(StrEqual(vmt_varname, "insert") ||
						vmt_cond_regex.Match(vmt_varname) > 0) {
				if(find_pathstring(processed_materials, vmt_varvalue) == -1) {
					push_pathstr(processed_materials, vmt_varvalue);

					parse_material_dl(mat);
				}
			} else if(StrEqual(vmt_varname, "include")) {
				if(find_pathstring(processed_materials, vmt_varvalue) == -1) {
					push_pathstr(processed_materials, vmt_varvalue);

					KeyValues inc = new KeyValues("");
					if(inc.ImportFromFile(vmt_varvalue)) {
						parse_material_dl(inc);
					}
					delete inc;
				}
			}
		} while(mat.GotoNextKey(false));
		mat.GoBack();
	}
}

static void add_incmdl_to_dltable(const char[] filename)
{
	AddFileToDownloadsTable_fixed(filename);

	int idx = ModelInfo.GetModelIndex(filename);
	if(idx == -1) {
		idx = PrecacheModel(filename);
	}

	StudioModel mdl = view_as<StudioModel>(ModelInfo.GetModel(idx));
	if(mdl == StudioModel_Null) {
		return;
	}

	mdl.GetAnimBlockPath(vmt_varvalue, PLATFORM_MAX_PATH);
	if(vmt_varvalue[0] != '\0') {
		AddFileToDownloadsTable_fixed(vmt_varvalue);
	}

	int num_incmdls = mdl.IncludedModels;
	for(int i = 0; i < num_incmdls; ++i) {
		mdl.GetIncludedModelPath(i, vmt_varvalue, PLATFORM_MAX_PATH);
		if(find_pathstring(processed_incmdls, vmt_varvalue) == -1) {
			push_pathstr(processed_incmdls, vmt_varvalue);
			add_incmdl_to_dltable(vmt_varvalue);
		}
	}
}

static void maintain_controls(bool remove_all)
{
	StringMapSnapshot snap = mat_ent_map.Snapshot();

	char material[MATERIAL_MODIFY_STRING_SIZE];
	char variable[MATERIAL_MODIFY_STRING_SIZE];
	char value[MATERIAL_MODIFY_STRING_SIZE];

	EntityMatVarInfo info;
	MatVarInfo var_info;

	int len = snap.Length;
	for(int j = 0; j < len; ++j) {
		snap.GetKey(j, material, MATERIAL_MODIFY_STRING_SIZE);

		ArrayList infos;
		if(mat_ent_map.GetValue(material, infos)) {
			int len2 = infos.Length;
			for(int i = 0; i < len2; ++i) {
				infos.GetArray(i, info, sizeof(EntityMatVarInfo));

				int target = EntRefToEntIndex(info.ref);

				StringMapSnapshot snap2 = info.variables.Snapshot();

				int len3 = snap2.Length;
				int len3_copy = len3;
				for(int k = 0; k < len3; ++k) {
					snap2.GetKey(k, variable, MATERIAL_MODIFY_STRING_SIZE);

					if(info.variables.GetArray(variable, var_info, sizeof(MatVarInfo))) {
						int control = EntRefToEntIndex(var_info.control);

						if(target == -1 || remove_all) {
						#if defined DEBUG
							PrintToServer("removing %s %i %s target was deleted", material, i, variable);
						#endif
							if(control != -1) {
								RemoveEntity(control);
							}
							info.variables.Remove(variable);
							continue;
						}

						if(control == -1) {
						#if defined DEBUG
							PrintToServer("created control entity for %s %i %s", material, i, variable);
						#endif

							control = CreateEntityByName("material_modify_control");
							DispatchKeyValue(control, "materialName", material);
							DispatchKeyValue(control, "materialVar", variable);
							DispatchSpawn(control);

							SetVariantString("!activator");
							AcceptEntityInput(control, "SetParent", target);

							SetVariantString(var_info.value);
							AcceptEntityInput(control, "SetMaterialVar");

							var_info.control = EntIndexToEntRef(control);

							info.variables.SetArray(variable, var_info, sizeof(MatVarInfo));
						} else {
							GetEntPropString(control, Prop_Send, "m_szMaterialVarValue", value, MATERIAL_MODIFY_STRING_SIZE);
							if(!StrEqual(value, var_info.value)) {
								SetVariantString(var_info.value);
								AcceptEntityInput(control, "SetMaterialVar");
							}
						}

						float life = (GetGameTime() - var_info.time);
						if(life >= 0.1 || remove_all) {
						#if defined DEBUG
							PrintToServer("removing %s %i %s time was expired", material, i, variable);
						#endif
							--len3_copy;
							if(control != -1) {
								RemoveEntity(control);
							}
							info.variables.Remove(variable);
							continue;
						}
					}
				}

				delete snap2;

				if(target == -1 ||
					len3_copy == 0 ||
					remove_all) {
				#if defined DEBUG
					PrintToServer("removing %s %i no variables left", material, i);
				#endif
					--len2;
					infos.Erase(i);
					continue;
				}
			}
			if(len2 == 0 ||
				remove_all) {
			#if defined DEBUG
				PrintToServer("removing %s no entites left", material);
			#endif
				delete infos;
				mat_ent_map.Remove(material);
			}
		}
	}

	delete snap;
}

static Action timer_maintain_controls(Handle timer, any data)
{
	maintain_controls(false);

	return Plugin_Continue;
}

public void OnPluginEnd()
{
	maintain_controls(true);
}

public void OnMapStart()
{
	CreateTimer(0.1, timer_maintain_controls, 0, TIMER_FLAG_NO_MAPCHANGE|TIMER_REPEAT);
}

public void OnMapEnd()
{
	maintain_controls(true);

	processed_materials.Clear();
	processed_incmdls.Clear();
}

static int native_AddModelToDownloadsTable(Handle plugin, int params)
{
	int len;
	GetNativeStringLength(1, len);
	char[] filename = new char[++len];
	GetNativeString(1, filename, len);

	AddFileToDownloadsTable_fixed(filename);

	int idx = ModelInfo.GetModelIndex(filename);
	if(idx == -1) {
		idx = PrecacheModel(filename);
	}

	int mdl_str_idx = StrContains(filename, ".mdl");
	if(mdl_str_idx != -1) {
		filename[mdl_str_idx] = '\0';
	}

	//TODO!!!! get the path from the model itself

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.vvd", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable_fixed(vmt_varvalue);
	}

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.phy", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable_fixed(vmt_varvalue);
	}

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.vtx", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable_fixed(vmt_varvalue);
	}

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.dx90.vtx", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable_fixed(vmt_varvalue);
	}

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.dx80.vtx", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable_fixed(vmt_varvalue);
	}

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.sw.vtx", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable_fixed(vmt_varvalue);
	}

	StudioModel mdl = view_as<StudioModel>(ModelInfo.GetModel(idx));
	if(mdl == StudioModel_Null) {
		return 0;
	}

	mdl.GetAnimBlockPath(vmt_varvalue, PLATFORM_MAX_PATH);
	if(vmt_varvalue[0] != '\0') {
		AddFileToDownloadsTable_fixed(vmt_varvalue);
	}

	push_pathstr(processed_incmdls, filename);
	int num_incmdls = mdl.IncludedModels;
	for(int i = 0; i < num_incmdls; ++i) {
		mdl.GetIncludedModelPath(i, vmt_varvalue, PLATFORM_MAX_PATH);
		push_pathstr(processed_incmdls, vmt_varvalue);
		add_incmdl_to_dltable(vmt_varvalue);
	}

	int num_texs = mdl.NumTextures;

	int num_lods = mdl.LODCount;
	for(int j = 0; j < num_lods; ++j) {
		StudioModelLOD lod = mdl.GetLOD(j);

		int num_mat = lod.MaterialCount;
		for(int i = 0; i < num_mat; ++i) {
			lod.GetMaterialName(i, vmt_varvalue, PLATFORM_MAX_PATH);
			if(StrEqual(vmt_varvalue, "___error")) {
				if(i < num_texs) {
					mdl.GetTextureName(i, vmt_varvalue, PLATFORM_MAX_PATH);
					LogMessage("model '%s' LOD#%i material#%i (possibly: '%s') is missing", filename, j, i, vmt_varvalue);
				} else {
					LogMessage("model '%s' LOD#%i material#%i is missing", filename, j, i);
				}
				continue;
			}

			if(find_pathstring(processed_materials, vmt_varvalue) != -1) {
				continue;
			}

			push_pathstr(processed_materials, vmt_varvalue);

			Format(vmt_varvalue, PLATFORM_MAX_PATH, "materials/%s.vmt", vmt_varvalue);
			AddFileToDownloadsTable_fixed(vmt_varvalue);

			KeyValues mat = new KeyValues("");
			if(mat.ImportFromFile(vmt_varvalue)) {
				parse_material_dl(mat);
			}
			delete mat;
		}
	}

	return 0;
}

static void normalize_pathsep(char[] str)
{
	int i = 0;
	while(str[i] != '\0') {
		if(str[i] == '\\') {
			str[i] = '/';
		}
		++i;
	}
}

static int find_pathstring(ArrayList arr, const char[] input)
{
	char temp[PLATFORM_MAX_PATH];

	int input_len = strlen(input);

	int len = arr.Length;
	for(int i = 0; i < len; ++i) {
		arr.GetString(i, temp, PLATFORM_MAX_PATH);

		bool equal = true;

		for(int k = 0; k <= input_len && k < PLATFORM_MAX_PATH; ++k) {
			char c1 = CharToLower(input[k]);
			if(c1 == '\\') {
				c1 = '/'
			}

			char c2 = temp[k];

			if(c1 != c2) {
				equal = false;
				break;
			}
		}

		if(!equal) {
			continue;
		}

		return i;
	}

	return -1;
}

static void push_pathstr(ArrayList arr, const char[] input)
{
	char temp[PLATFORM_MAX_PATH];
	strcopy(temp, PLATFORM_MAX_PATH, input);

	lowerstr(temp);
	normalize_pathsep(temp);

	arr.PushString(temp);
}

static void AddFileToDownloadsTable_fixed(const char[] filename)
{
	//TODO!!!!!!! resolve full path

	char temp[PLATFORM_MAX_PATH];
	strcopy(temp, PLATFORM_MAX_PATH, filename);

	normalize_pathsep(temp);

	AddFileToDownloadsTable(temp);
}

static any native_set_material_var(Handle plugin, int params)
{
	char material[MATERIAL_MODIFY_STRING_SIZE];
	GetNativeString(2, material, MATERIAL_MODIFY_STRING_SIZE);

	ArrayList infos;
	if(!mat_ent_map.GetValue(material, infos)) {
		infos = new ArrayList(sizeof(EntityMatVarInfo));
		mat_ent_map.SetValue(material, infos);
	}

	int entity = GetNativeCell(1);
	int ref = EntIndexToEntRef(entity);

	EntityMatVarInfo info;

	int idx = infos.FindValue(ref, EntityMatVarInfo::ref);
	if(idx == -1) {
		info.ref = ref;
		info.variables = new StringMap();
		idx = infos.PushArray(info, sizeof(EntityMatVarInfo));
	} else {
		infos.GetArray(idx, info, sizeof(EntityMatVarInfo));
	}

	char variable[MATERIAL_MODIFY_STRING_SIZE];
	GetNativeString(3, variable, MATERIAL_MODIFY_STRING_SIZE);

	char value[MATERIAL_MODIFY_STRING_SIZE];
	GetNativeString(4, value, MATERIAL_MODIFY_STRING_SIZE);


	MatVarInfo var_info;

	if(!info.variables.GetArray(variable, var_info, sizeof(MatVarInfo))) {
		var_info.control = INVALID_ENT_REFERENCE;
	}

	strcopy(var_info.value, MATERIAL_MODIFY_STRING_SIZE, value);
	var_info.time = GetGameTime();

	info.variables.SetArray(variable, var_info, sizeof(MatVarInfo));

	return 0;
}