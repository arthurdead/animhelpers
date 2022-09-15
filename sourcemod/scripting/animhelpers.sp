#include <sourcemod>
#include <animhelpers>

static char vmt_varname[64];
static char vmt_varvalue[PLATFORM_MAX_PATH];

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int length)
{
	RegPluginLibrary("animhelpers");
	CreateNative("AddModelToDownloadsTable", native_AddModelToDownloadsTable);
	return APLRes_Success;
}

static Regex vmt_cond_regex;

public void OnPluginStart()
{
	char err[128];
	RegexError errcode = REGEX_ERROR_NONE;
	vmt_cond_regex = new Regex(
		"(?:[a-z]_)?(?:(?:[<>]=?)?dx(?:9[05]?|6)(?:_20b)?|ldr|srgb|hdr)"
		, 0, err, sizeof(err), errcode
	);
	if(errcode != REGEX_ERROR_NONE) {
		SetFailState("failed to create vmt conditional regex: %s (%i)", err, errcode);
	}
}

static void lowerstr(char[] name)
{
	int i = 0;
	while(name[i++] != '\0') {
		name[i] = CharToLower(name[i]);
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
				StrEqual(vmt_varname, "$detail")) {
				int ext = StrContains(vmt_varvalue, ".vtf");
				if(ext != -1) {
					vmt_varvalue[ext] = '\0';
				}

				Format(vmt_varvalue, PLATFORM_MAX_PATH, "materials/%s.vtf", vmt_varvalue);
				AddFileToDownloadsTable(vmt_varvalue);
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

					Format(vmt_varvalue, PLATFORM_MAX_PATH, "materials/%s.vtf", vmt_varvalue);
					if(FileExists(vmt_varvalue, true)) {
						AddFileToDownloadsTable(vmt_varvalue);
					}
					Format(vmt_varvalue, PLATFORM_MAX_PATH, "materials/%s.hdr.vtf", vmt_varvalue);
					if(FileExists(vmt_varvalue, true)) {
						AddFileToDownloadsTable(vmt_varvalue);
					}
				}
			} else if(StrEqual(vmt_varname, "$material") ||
						StrEqual(vmt_varname, "$modelmaterial")) {
				int ext = StrContains(vmt_varvalue, ".vmt");
				if(ext != -1) {
					vmt_varvalue[ext] = '\0';
				}

				Format(vmt_varvalue, PLATFORM_MAX_PATH, "materials/%s.vmt", vmt_varvalue);
				AddFileToDownloadsTable(vmt_varvalue);
			} else if(StrEqual(vmt_varname, "insert") ||
						vmt_cond_regex.Match(vmt_varname) > 0) {
				parse_material_dl(mat);
			} else if(StrEqual(vmt_varname, "include")) {
				KeyValues inc = new KeyValues("");
				if(inc.ImportFromFile(vmt_varvalue)) {
					parse_material_dl(inc);
				}
				delete inc;
			}
		} while(mat.GotoNextKey(false));
		mat.GoBack();
	}
}

static void add_incmdl_to_dltable(const char[] filename)
{
	AddFileToDownloadsTable(filename);

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
		AddFileToDownloadsTable(vmt_varvalue);
	}

	int num_incmdls = mdl.IncludedModels;
	for(int i = 0; i < num_incmdls; ++i) {
		mdl.GetIncludedModelPath(i, vmt_varvalue, PLATFORM_MAX_PATH);
		add_incmdl_to_dltable(vmt_varvalue);
	}
}

static int native_AddModelToDownloadsTable(Handle plugin, int params)
{
	int len;
	GetNativeStringLength(1, len);
	char[] filename = new char[++len];
	GetNativeString(1, filename, len);

	AddFileToDownloadsTable(filename);

	int idx = ModelInfo.GetModelIndex(filename);
	if(idx == -1) {
		idx = PrecacheModel(filename);
	}

	int mdl_str_idx = StrContains(filename, ".mdl");
	if(mdl_str_idx != -1) {
		filename[mdl_str_idx] = '\0';
	}

	//TODO!!!! get the path from the model itself

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.vdd", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable(vmt_varvalue);
	}

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.phy", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable(vmt_varvalue);
	}

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.vtx", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable(vmt_varvalue);
	}

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.dx90.vtx", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable(vmt_varvalue);
	}

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.dx80.vtx", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable(vmt_varvalue);
	}

	FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "%s.sw.vtx", filename);
	if(FileExists(vmt_varvalue, true)) {
		AddFileToDownloadsTable(vmt_varvalue);
	}

	StudioModel mdl = view_as<StudioModel>(ModelInfo.GetModel(idx));
	if(mdl == StudioModel_Null) {
		return 0;
	}

	mdl.GetAnimBlockPath(vmt_varvalue, PLATFORM_MAX_PATH);
	if(vmt_varvalue[0] != '\0') {
		AddFileToDownloadsTable(vmt_varvalue);
	}

	int num_incmdls = mdl.IncludedModels;
	for(int i = 0; i < num_incmdls; ++i) {
		mdl.GetIncludedModelPath(i, vmt_varvalue, PLATFORM_MAX_PATH);
		add_incmdl_to_dltable(vmt_varvalue);
	}

	int num_lods = mdl.LODCount;
	for(int j = 0; j < num_lods; ++j) {
		StudioModelLOD lod = mdl.GetLOD(j);

		int num_mat = lod.MaterialCount;
		for(int i = 0; i < num_mat; ++i) {
			lod.GetMaterialName(i, vmt_varvalue, PLATFORM_MAX_PATH);
			FormatEx(vmt_varvalue, PLATFORM_MAX_PATH, "materials/%s.vmt", vmt_varvalue);
			AddFileToDownloadsTable(vmt_varvalue);

			KeyValues mat = new KeyValues("");
			if(mat.ImportFromFile(vmt_varvalue)) {
				parse_material_dl(mat);
			}
			delete mat;
		}
	}

	return 0;
}