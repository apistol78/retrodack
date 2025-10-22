#if defined(__RV__)

#include "stdafx.h"
#include "../fs.h"

#include <algorithm>
#include <string>

#include <stdio.h>
#include <stdlib.h>

#include <Runtime/File.h>

static const char *lastPathComponent(const Common::String &str)
{
	const char *start = str.c_str();
	const char *cur = start + str.size() - 2;
	
	while (cur > start && *cur != '/')
	{
		--cur;
	}
	
	return cur+1;
}

class RvFilesystemNode : public FilesystemNode
{
public:
	RvFilesystemNode() = default;
	RvFilesystemNode(const RvFilesystemNode *node);

	virtual String displayName() const { return m_displayName; }
	virtual bool isValid() const { return true; }
	virtual bool isDirectory() const { return m_isDirectory; }
	virtual String path() const { return m_path; }

	virtual FSList *listDir(ListMode) const;
	virtual FilesystemNode *parent() const;
	virtual FilesystemNode *clone() const { return new RvFilesystemNode(this); }

private:
	String m_path = "";
	String m_displayName = "";
	bool m_isDirectory = true;
};

FilesystemNode *FilesystemNode::getRoot()
{
	return new RvFilesystemNode();
}

RvFilesystemNode::RvFilesystemNode(const RvFilesystemNode* node)
:	m_path(node->m_path)
,	m_displayName(node->m_displayName)
,	m_isDirectory(node->m_isDirectory)
{
}

FSList *RvFilesystemNode::listDir(ListMode mode) const
{
	FSList* list = new FSList();

	struct User
	{
		const RvFilesystemNode* parent;
		FSList* list;
	}
	user = { this, list };

	file_enumerate(m_path.c_str(), &user, [](void* user, const char* filename, uint32_t size, uint8_t directory)
	{
		User* up = (User*)user;

		if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
			return;

		printf("..RvFilesystemNode, adding file \"%s\"\n", filename);

		RvFilesystemNode entry;

		entry.m_path = up->parent->m_path;
		if (!entry.m_path.isEmpty())
			entry.m_path += "/";
		entry.m_path += filename;

		entry.m_displayName = filename;
		entry.m_isDirectory = (bool)(directory != 0);

		up->list->push_back(entry);
	});

	return list;
}

FilesystemNode *RvFilesystemNode::parent() const
{
	RvFilesystemNode* p = new RvFilesystemNode();

	if (m_path != "/")
	{
		const char *start = m_path.c_str();
		const char *end = lastPathComponent(m_path);

		p->m_path = String(start, end - start);
		p->m_displayName = lastPathComponent(p->m_path);
	}
	else
	{
		p->m_path = m_path;
		p->m_displayName = m_displayName;
	}

	p->m_isDirectory = true;
	return p;
}

#endif // __RV__
