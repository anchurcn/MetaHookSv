#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/IEngineVGui.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include "BaseUI.h"
#include "viewport.h"
#include "SubtitlePanel.h"
#include "cstrikechatdialog.h"
#include "MemPool.h"
#include "message.h"
#include "privatefuncs.h"
#include "exportfuncs.h"
#include <stdexcept>

using namespace vgui;

CViewport *g_pViewPort = NULL;

//Dictionary hashtable
CMemoryPool m_HashItemMemPool(sizeof(hash_item_t), 64);

extern CHudMessage m_HudMessage;

CViewport::CViewport(void) : Panel(NULL, "CaptionViewport")
{
	int swide, stall;
	surface()->GetScreenSize(swide, stall);

	MakePopup(false, true);

	SetScheme("CaptionScheme");
	SetBounds(0, 0, swide, stall);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
	SetProportional(true);
	m_pSubtitlePanel = NULL;
	m_szLevelName[0] = 0;
}

CViewport::~CViewport(void)
{
	for (int i = 0; i < m_Dictionary.Count(); ++i)
	{
		delete m_Dictionary[i];
	}

	m_Dictionary.RemoveAll();

	delete m_pSubtitlePanel;
}

CDictionary *CViewport::FindDictionary(const char *szValue)
{
	if (!m_Dictionary.Count())
		return NULL;

	int hash = 0;
	hash_item_t *item;
	int count;

	hash = CaseInsensitiveHash(szValue, m_StringsHashTable.Count());
	count = m_StringsHashTable.Count();
	item = &m_StringsHashTable[hash];

	while (item->dict)
	{
		if (!Q_strcmp(item->dict->m_szTitle.c_str(), szValue))
			break;

		hash = (hash + 1) % count;
		item = &m_StringsHashTable[hash];
	}

	if (!item->dict)
	{
		item->lastHash = NULL;
		return NULL;
	}

	m_StringsHashTable[hash].lastHash = item;
	return item->dict;
}

CDictionary *CViewport::FindDictionary(const char *szValue, dict_t Type)
{
	if (!m_Dictionary.Count())
		return NULL;

	int hash = 0;
	hash_item_t *item;
	int count;

	hash = CaseInsensitiveHash(szValue, m_StringsHashTable.Count());
	count = m_StringsHashTable.Count();
	item = &m_StringsHashTable[hash];

	while (item->dict)
	{
		if (!Q_strcmp(item->dict->m_szTitle.c_str(), szValue) && item->dict->m_Type == Type)
			break;

		hash = (hash + 1) % count;
		item = &m_StringsHashTable[hash];
	}

	if (!item->dict)
	{
		item->lastHash = NULL;
		return NULL;
	}

	m_StringsHashTable[hash].lastHash = item;
	return item->dict;
}

CDictionary *CViewport::FindDictionaryRegex(const std::string &str, dict_t Type, std::smatch &result)
{
	if (!m_Dictionary.Count())
		return NULL;

	for(int i = 0; i < m_Dictionary.Count(); ++i)
	{
		if (m_Dictionary[i]->m_Type == Type && m_Dictionary[i]->m_bRegex)
		{
			std::regex pattern(m_Dictionary[i]->m_szTitle);

			if (std::regex_search(str, result, pattern))
			{
				return m_Dictionary[i];
			}
		}
	}

	return NULL;
}

int CViewport::CaseInsensitiveHash(const char *string, int iBounds)
{
	unsigned int hash = 0;

	if (!*string)
		return 0;

	while (*string)
	{
		if (*string < 'A' || *string > 'Z')
			hash = *string + 2 * hash;
		else
			hash = *string + 2 * hash + ' ';

		string++;
	}

	return (hash % iBounds);
}

void CViewport::EmptyDictionaryHash(void)
{
	int i;
	hash_item_t *item;
	hash_item_t *temp;
	hash_item_t *free;

	for (i = 0; i < m_StringsHashTable.Count(); i++)
	{
		item = &m_StringsHashTable[i];
		temp = item->next;
		item->dict = NULL;
		item->dictIndex = 0;
		item->lastHash = NULL;
		item->next = NULL;

		while (temp)
		{
			free = temp;
			temp = temp->next;
			m_HashItemMemPool.Free(free);
		}
	}
}

void CViewport::AddDictionaryHash(CDictionary *dict, const char *value)
{
	int count;
	hash_item_t *item;
	hash_item_t *next;
	hash_item_t *temp;
	hash_item_t *newp;
	unsigned int hash = 0;
	int dictIndex;
	CDictionary *dictTemp;

	if (!dict->m_szTitle[0])
		return;

	count = m_StringsHashTable.Count();
	hash = CaseInsensitiveHash(value, count);
	dictIndex = dict - m_Dictionary[0];

	item = &m_StringsHashTable[hash];

	while (item->dict)
	{
		if (!Q_strcmp(item->dict->m_szTitle.c_str(), dict->m_szTitle.c_str()))
			break;

		hash = (hash + 1) % count;
		item = &m_StringsHashTable[hash];
	}

	if (item->dict)
	{
		next = item->next;

		while (next)
		{
			if (item->dict == dict)
				break;

			if (item->dictIndex >= dictIndex)
				break;

			item = next;
			next = next->next;
		}

		if (dictIndex < item->dictIndex)
		{
			dictTemp = item->dict;
			item->dict = dict;
			item->lastHash = NULL;
			item->dictIndex = dictIndex;
			dictIndex = dictTemp - m_Dictionary[0];
		}
		else
			dictTemp = dict;

		if (item->dict != dictTemp)
		{
			temp = item->next;
			newp = (hash_item_t *)m_HashItemMemPool.Alloc(sizeof(hash_item_t));
			item->next = newp;
			newp->dict = dictTemp;
			newp->lastHash = NULL;
			newp->dictIndex = dictIndex;

			if (temp)
				newp->next = temp;
			else
				newp->next = NULL;
		}
	}
	else
	{
		item->dict = dict;
		item->lastHash = NULL;
		item->dictIndex = dict - m_Dictionary[0];
	}
}

void CViewport::RemoveDictionaryHash(CDictionary *dict, const char *value)
{
	int hash = 0;
	hash_item_t *item;
	hash_item_t *last;
	int dictIndex;
	int count;

	count = m_StringsHashTable.Count();
	hash = CaseInsensitiveHash(value, count);
	dictIndex = dict - m_Dictionary[0];


	hash = hash % count;
	item = &m_StringsHashTable[hash];

	while (item->dict)
	{
		if (!Q_strcmp(item->dict->m_szTitle.c_str(), dict->m_szTitle.c_str()))
			break;

		hash = (hash + 1) % count;
		item = &m_StringsHashTable[hash];
	}

	if (item->dict)
	{
		last = item;

		while (item->next)
		{
			if (item->dict == dict)
				break;

			last = item;
			item = item->next;
		}

		if (item->dict == dict)
		{
			if (last == item)
			{
				if (item->next)
				{
					item->dict = item->next->dict;
					item->dictIndex = item->next->dictIndex;
					item->lastHash = NULL;
					item->next = item->next->next;
				}
				else
				{
					item->dict = NULL;
					item->lastHash = NULL;
					item->dictIndex = 0;
				}
			}
			else
			{
				if (m_StringsHashTable[hash].lastHash == item)
					m_StringsHashTable[hash].lastHash = NULL;

				last->next = item->next;
				m_HashItemMemPool.Free(item);
			}
		}
	}
}

CDictionary::CDictionary()
{
	m_Type = DICT_CUSTOM;
	m_Color = Color(255, 255, 255, 255);
	m_flDuration = 0;
	m_flNextDelay = 0;
	m_pNext = NULL;
	m_iTextAlign = ALIGN_DEFAULT;
	m_bRegex = false;
	m_bOverrideColor = false;
	m_bOverrideDuration = false;
}

CDictionary::~CDictionary()
{

}

void StringReplaceW(std::wstring &strBase, const std::wstring &strSrc, const std::wstring &strDst)
{
	size_t pos = 0;
	auto srcLen = strSrc.size();
	auto desLen = strDst.size();
	pos = strBase.find(strSrc, pos);
	while ((pos != std::wstring::npos))
	{
		strBase.replace(pos, srcLen, strDst);
		pos = strBase.find(strSrc, (pos + desLen));
	}
}

void StringReplaceA(std::string &strBase, const std::string &strSrc, const std::string &strDst)
{
	size_t pos = 0;
	auto srcLen = strSrc.size();
	auto desLen = strDst.size();
	pos = strBase.find(strSrc, pos);
	while ((pos != std::string::npos))
	{
		strBase.replace(pos, srcLen, strDst);
		pos = strBase.find(strSrc, (pos + desLen));
	}
}

void CDictionary::Load(CSV::CSVDocument::row_type &row, Color &defaultColor, IScheme *ischeme)
{
	m_Color = defaultColor;

	m_szTitle = row[0];

	//If title ended with .wav

	if(m_szTitle.length() > 4 && !Q_stricmp(&m_szTitle[m_szTitle.length() - 4], ".wav"))
	{
		m_Type = DICT_SOUND;
	}

	//If it's a textmessage found in engine (gamedir/titles.txt)
	client_textmessage_t *textmsg = gEngfuncs.pfnTextMessageGet(m_szTitle[0] == '#' ? &m_szTitle[1] : &m_szTitle[0]);
	if (textmsg)
	{
		m_Type = DICT_MESSAGE;
	}
	else if (m_szTitle[0] == '%' && m_szTitle[1] == '!')
	{
		m_Type = DICT_SENDAUDIO;
	}
	else if (m_szTitle[0] == '!' || m_szTitle[0] == '#')
	{
		m_Type = DICT_SENTENCE;
	}

	//2015-11-26 added to support NETMESSAGE:
	if (!Q_strncmp(m_szTitle.c_str(), "NETMESSAGE_REGEX:", sizeof("NETMESSAGE_REGEX:") - 1))
	{
		m_Type = DICT_NETMESSAGE;
		m_szTitle = m_szTitle.substr(sizeof("NETMESSAGE_REGEX:") - 1);
		m_bRegex = true;
	}
	else if (!Q_strncmp(m_szTitle.c_str(), "NETMESSAGE:", sizeof("NETMESSAGE:") - 1))
	{
		m_Type = DICT_NETMESSAGE;
		m_szTitle = m_szTitle.substr(sizeof("NETMESSAGE:") - 1);
		m_bRegex = false;
	}
	else if (!Q_strncmp(m_szTitle.c_str(), "MESSAGE:", sizeof("MESSAGE:") - 1))
	{
		m_Type = DICT_MESSAGE;
		m_szTitle = m_szTitle.substr(sizeof("MESSAGE:") - 1);
		m_bRegex = false;
	}
	else if (!Q_strncmp(m_szTitle.c_str(), "SENTENCE:", sizeof("SENTENCE:") - 1))
	{
		m_Type = DICT_SENTENCE;
		m_szTitle = m_szTitle.substr(sizeof("SENTENCE:") - 1);
		m_bRegex = false;
	}
	else if (!Q_strncmp(m_szTitle.c_str(), "SENDAUDIO:", sizeof("SENDAUDIO:") - 1))
	{
		m_Type = DICT_SENDAUDIO;
		m_szTitle = m_szTitle.substr(sizeof("SENDAUDIO:") - 1);
		m_bRegex = false;
	}

	//Translated text
	const char *sentence = row[1].c_str();
	wchar_t* pLocalized = NULL;
	if (sentence[0] == '#')
	{
		pLocalized = localize()->Find(sentence);
		if (pLocalized)
		{
			int localizedLength = Q_wcslen(pLocalized);
			m_szSentence.resize(localizedLength);
			memcpy(&m_szSentence[0], pLocalized, (localizedLength + 1) * sizeof(wchar_t));
		}
	}
	if (!pLocalized)
	{
		int localizedLength = MultiByteToWideChar(CP_ACP, 0, sentence, -1, NULL, 0);
		m_szSentence.resize(localizedLength - 1);
		MultiByteToWideChar(CP_ACP, 0, sentence, -1, &m_szSentence[0], localizedLength);
	}

	if (m_Type == DICT_NETMESSAGE && !m_bRegex)
	{
		StringReplaceA(m_szTitle, "\\n", "\n");
		StringReplaceA(m_szTitle, "\\r", "\r");
	}
	StringReplaceW(m_szSentence, L"\\n", L"\n");
	StringReplaceW(m_szSentence, L"\\r", L"\r");

	const char *color = row[2].c_str();

	if(color[0])
	{
		CUtlVector<char *> splitColor;
		V_SplitString(color, " ", splitColor);

		m_Color = ischeme->GetColor(color, defaultColor);

		if(splitColor.Size() >= 2)
		{
			if(splitColor[0][0])
			{
				m_Color1 = ischeme->GetColor(splitColor[0], defaultColor);
			}
			if(splitColor[1][0])
			{
				m_Color2 = ischeme->GetColor(splitColor[1], defaultColor);
			}

			m_bOverrideColor = true;
		}

		splitColor.PurgeAndDeleteElements();
	}

	const char *duration = row[3].c_str();
	if(duration[0])
	{
		m_flDuration = Q_atof(duration);

		if(m_flDuration > 0)
			m_bOverrideDuration = true;
	}

	const char *speaker = row[4].c_str();
	if(speaker[0])
	{
		if(speaker[0] == '#')
		{
			pLocalized = localize()->Find(speaker);
			if (pLocalized)
			{
				int localizedLength = Q_wcslen(pLocalized);
				m_szSpeaker.resize(localizedLength);
				memcpy(&m_szSpeaker[0], pLocalized, (localizedLength + 1) * sizeof(wchar_t));
			}
		}
		if (!pLocalized)
		{
			int localizedLength = MultiByteToWideChar(CP_ACP, 0, speaker, -1, NULL, 0);
			m_szSpeaker.resize(localizedLength - 1);
			MultiByteToWideChar(CP_ACP, 0, speaker, -1, &m_szSpeaker[0], localizedLength);
		}
	}

	//Next dictionary
	if(row.size() >= 7)
	{
		m_szNext = row[5];

		const char* nextdelay = row[6].c_str();

		if (nextdelay[0])
		{
			m_flNextDelay = Q_atof(nextdelay);
		}
	}

	//Text alignment
	if(row.size() >= 8)
	{
		const char *textalign = row[7].c_str();
		if(textalign[0] == 'R' || textalign[0] == 'r')
			m_iTextAlign = ALIGN_RIGHT;
		else if(textalign[0] == 'C' || textalign[0] == 'c')
			m_iTextAlign = ALIGN_CENTER;
		if(textalign[0] == 'L' || textalign[0] == 'l')
			m_iTextAlign = ALIGN_LEFT;
	}
}

void CViewport::LoadCustomDictionary(const char *dict_name)
{
	CSV::CSVDocument doc;
	CSV::CSVDocument::row_index_type row_count = 0;

	//Parse from the document

	try
	{
		row_count = doc.load_file(dict_name);
	}
	catch (std::exception &err)
	{
		gEngfuncs.Con_Printf("LoadCustomDictionary: %s\n", err.what());
	}

	if (row_count < 2)
	{
		gEngfuncs.Con_Printf("LoadCustomDictionary: too few lines in the dictionary file.\n");
		return;
	}

	IScheme *ischeme = scheme()->GetIScheme(GetScheme());

	if (!ischeme)
		return;

	Color defaultColor = ischeme->GetColor("BaseText", Color(255, 255, 255, 200));

	int nRowCount = row_count;

	//parse the dictionary line by line...
	for (int i = 1; i < nRowCount; ++i)
	{
		CSV::CSVDocument::row_type row = doc.get_row(i);

		if (row.size() < 1)
			continue;

		const char *title = row[0].c_str();

		if (!title || !title[0])
			continue;

		CDictionary *Dict = new CDictionary;

		Dict->Load(row, defaultColor, ischeme);

		m_Dictionary.AddToTail(Dict);

		AddDictionaryHash(Dict, Dict->m_szTitle.c_str());
	}

	gEngfuncs.Con_Printf("LoadCustomDictionary: %d lines are loaded.\n", nRowCount-1);
}

void CViewport::LinkDictionary(void)
{
	for (int i = 0; i < m_Dictionary.Count(); ++i)
	{
		CDictionary *Dict = m_Dictionary[i];
		if (Dict->m_szNext[0])
		{
			Dict->m_pNext = FindDictionary(Dict->m_szNext.c_str());
		}
	}
}

void CViewport::LoadBaseDictionary(void)
{
	CSV::CSVDocument doc;
	CSV::CSVDocument::row_index_type row_count = 0;

	//Parse from the document

	try
	{
		row_count = doc.load_file("captionmod/dictionary.csv");
	}
	catch(std::exception &err)
	{
		g_pMetaHookAPI->SysError("LoadBaseDictionary: %s\n", err.what());
	}

	if (row_count < 2)
	{
		gEngfuncs.Con_Printf("LoadBaseDictionary: too few lines in the dictionary file.\n");
		return;
	}

	IScheme *ischeme = scheme()->GetIScheme(GetScheme());

	if(!ischeme)
		return;

	Color defaultColor = ischeme->GetColor("BaseText", Color(255, 255, 255, 200));
	
	//Initialize the dictionary hashtable
	m_StringsHashTable.SetSize(2048);

	for (int i = 0; i < m_StringsHashTable.Count(); i++)
		m_StringsHashTable[i].next = NULL;

	EmptyDictionaryHash();

	int nRowCount = row_count;
	
	//parse the dictionary line by line...
	for (int i = 1;i < nRowCount; ++i)
	{
		CSV::CSVDocument::row_type row = doc.get_row(i);

		if(row.size() < 1)
			continue;

		const char *title = row[0].c_str();

		if(!title || !title[0])
			continue;

		CDictionary *Dict = new CDictionary;

		Dict->Load(row, defaultColor, ischeme);

		m_Dictionary.AddToTail(Dict);

		AddDictionaryHash(Dict, Dict->m_szTitle.c_str());
	}

	gEngfuncs.Con_Printf("LoadBaseDictionary: %d lines are loaded.\n", nRowCount - 1);
}

extern char *m_pSenderName;

//KeyBinding Name(jump) -> Key Name(SPACE)
const char *PrimaryKey_ForBinding(const char *binding)
{
	if(binding[0] == '+')
		binding ++;

	if (!strcmp(binding, "sender") && m_pSenderName)
	{
		return m_pSenderName;
	}

	for (int i = 255; i >= 0; --i)
	{
		const char *found = gameuifuncs->Key_BindingForKey(i);

		if(found && found[0])
		{
			if(found[0] == '+')
				found ++;
			if(!Q_stricmp(found, binding))
			{
				const char *key = gameuifuncs->Key_NameForKey(i);
				if(key && key[0])
				{
					return key;
				}
			}
		}
	}
	return "<not bound>";
}

void CDictionary::FinalizeString(std::wstring &output, int iPrefix)
{
	auto finalize = m_szSentence;

	std::wregex pattern(L"(<([A-Za-z_]+)>)");
	std::wsmatch result;
	std::regex_search(output, result, pattern);

	std::wstring skipped;

	std::wstring::const_iterator searchStart(finalize.cbegin());

	while (std::regex_search(searchStart, finalize.cend(), result, pattern) && result.size() > 2)
	{
		std::wstring prefix = result.prefix();
		std::wstring suffix = result.suffix();

		auto wkeybind = result[2].str();

		char akeybind[256] = {0};
		g_pVGuiLocalize->ConvertUnicodeToANSI(wkeybind.c_str(), akeybind, sizeof(akeybind) - 1);
		const char *pszBinding = PrimaryKey_ForBinding(akeybind);

		if (pszBinding)
		{
			wchar_t wbinding[256] = {0};
			Q_UTF8ToUnicode(pszBinding, wbinding, sizeof(wbinding) - sizeof(wchar_t));

			if (searchStart != finalize.cbegin())
			{
				finalize = skipped + prefix;
			}
			else
			{
				finalize = prefix;
			}
			finalize += wbinding;

			auto currentLength = finalize.length();

			finalize += suffix;

			skipped = finalize.substr(0, currentLength);
			searchStart = finalize.cbegin() + currentLength;
			continue;
		}

		searchStart = result.suffix().first;
	}

	/*if (bReplaced && m_pTextMessage)
	{
		if (m_pTextMessage->pMessage)
			delete m_pTextMessage->pMessage;

		//Covert the text to UTF8
		int utf8Length = WideCharToMultiByte(CP_UTF8, 0, &output[0], -1, NULL, 0, NULL, NULL);
		char* utf8Text = new char[utf8Length + 1];
		WideCharToMultiByte(CP_UTF8, 0, &output[0], -1, utf8Text, utf8Length, NULL, NULL);
		utf8Text[utf8Length] = '\0';
		m_pTextMessage->pMessage = utf8Text;
	}*/

	if(iPrefix)
		output = m_szSpeaker + finalize;
	else
		output = finalize;
}

void CViewport::Start(void)
{
	m_pSubtitlePanel = new SubtitlePanel(NULL);
	m_pChatDialog = new CCSChatDialog(this);

	SetVisible(false);
}

void CViewport::SetParent(VPANEL vPanel)
{
	BaseClass::SetParent(vPanel);

	m_pSubtitlePanel->SetParent(this);
}

void CViewport::Think(void)
{
	auto levelname = gEngfuncs.pfnGetLevelName();
	if (!levelname || !levelname[0])
		return;

	if (0 != strcmp(levelname, m_szLevelName))
	{
		std::string name = levelname;
		name = name.substr(0, name.length() - 4);
		name += "_dictionary.csv";

		LoadCustomDictionary(name.c_str());
		LinkDictionary();

		strcpy(m_szLevelName, levelname);
	}
}

void CViewport::VidInit(void)
{
	m_szLevelName[0] = 0;
	LoadBaseDictionary();
	LinkDictionary();

	m_HudMessage.VidInit();
}

void CViewport::Init(void)
{
	m_HudMessage.Init();
}

void CViewport::StartSubtitle(CDictionary *dict)
{
	if (cap_enabled && cap_enabled->value) {
		m_pSubtitlePanel->StartSubtitle(dict, (*cl_time));
	}
}

void CViewport::StartNextSubtitle(CDictionary* dict)
{
	if (cap_enabled && cap_enabled->value) {
		m_pSubtitlePanel->StartNextSubtitle(dict);
	}
}

void CViewport::ActivateClientUI(void)
{
	SetVisible(true);
}

void CViewport::HideClientUI(void)
{
	SetVisible(false);
}

void CViewport::Paint(void)
{
	BaseClass::Paint();

	m_HudMessage.Draw();
}

bool CViewport::AllowedToPrintText(void)
{
	if (gPrivateFuncs.GameViewport_AllowedToPrintText)
		return gPrivateFuncs.GameViewport_AllowedToPrintText(GameViewport, 0);

	return true;
}

void CViewport::StartMessageMode(void)
{
	m_pChatDialog->StartMessageMode(MM_SAY);
}

void CViewport::StartMessageMode2(void)
{
	m_pChatDialog->StartMessageMode(MM_SAY_TEAM);
}

void CViewport::ChatPrintf(int iPlayerIndex, const wchar_t *buffer)
{
	m_pChatDialog->ChatPrintf(iPlayerIndex, buffer);
}

void CViewport::QuerySubtitlePanelVars(SubtitlePanelVars_t *vars)
{
	m_pSubtitlePanel->QuerySubtitlePanelVars(vars);
}

void CViewport::UpdateSubtitlePanelVars(SubtitlePanelVars_t *vars)
{
	m_pSubtitlePanel->UpdateSubtitlePanelVars(vars);
}