/*
 * niepce - db/library.cpp
 *
 * Copyright (C) 2007-2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <time.h>
#include <stdio.h>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>

#include "niepce/notifications.h"
#include "library.h"
#include "metadata.h"
#include "fwk/utils/exception.h"
#include "fwk/utils/exempi.h"
#include "fwk/utils/debug.h"
#include "fwk/utils/db/sqlite/sqlitecnxmgrdrv.h"
#include "fwk/utils/db/sqlite/sqlitecnxdrv.h"
#include "fwk/utils/db/sqlstatement.h"
#include "fwk/toolkit/notificationcenter.h"
#include "fwk/toolkit/mimetype.h"

using framework::NotificationCenter;

namespace bfs = boost::filesystem;

namespace db {

const char * s_databaseName = "niepcelibrary.db";


Library::Library(const std::string & dir, const NotificationCenter::Ptr & nc)
    : m_maindir(dir),
      m_dbname(m_maindir / s_databaseName),
      m_dbmgr(new db::sqlite::SqliteCnxMgrDrv()),
      m_notif_center(nc),
      m_inited(false)
{
    DBG_OUT("dir = %s", dir.c_str());
    db::DBDesc desc("", 0, m_dbname.string());
    m_dbdrv = m_dbmgr->connect_to_db(desc, "", "");
    m_inited = init();

    m_dbdrv->create_function0("rewrite_xmp", 
                              boost::bind(&Library::triggerRewriteXmp,
                                          this));
}

Library::~Library()
{
}

void Library::triggerRewriteXmp(void)
{
    DBG_OUT("rewrite_xmp");
    notify(NOTIFY_XMP_NEEDS_UPDATE, boost::any());
}

void Library::notify(NotifyType t, const boost::any & param)
{
    framework::NotificationCenter::Ptr nc(m_notif_center.lock());
    if(nc) {
        DBG_OUT("notif");
        // pass the notification
        framework::Notification::Ptr n(new framework::Notification(niepce::NOTIFICATION_LIB));
        framework::Notification::mutex_t::scoped_lock lock(n->mutex());
        LibNotification ln;
        ln.type = t;
        ln.param = param;
        n->setData(boost::any(ln));
        nc->post(n);
    }
    else {
        DBG_OUT("try to send a notification without notification center");
    }
}

/** init the database
 * @return true is the DB is inited. false if it fail.
 */
bool Library::init()
{
    int version = checkDatabaseVersion();
    if(version == -1) {
        // error
        DBG_OUT("version check -1");
    }
    else if(version == 0) {
        // let's create our DB
        DBG_OUT("version == 0");
        return _initDb();
    }
    else if(version != DB_SCHEMA_VERSION)
    {
    }
    return true;
}

bool Library::_initDb()
{
    SQLStatement adminTable("CREATE TABLE admin (key TEXT NOT NULL,"
                            " value TEXT)");
    SQLStatement adminVersion(boost::format("INSERT INTO admin (key, value) "
                                            " VALUES ('version', '%1%')") %
                              DB_SCHEMA_VERSION);
    SQLStatement vaultTable("CREATE TABLE vaults (id INTEGER PRIMARY KEY,"
                            " path TEXT)");
    SQLStatement folderTable("CREATE TABLE folders (id INTEGER PRIMARY KEY,"
                             " path TEXT, name TEXT, vault_id INTEGER, "
                             " parent_id INTEGER)");
    SQLStatement fileTable("CREATE TABLE files (id INTEGER PRIMARY KEY,"
                           " main_file INTEGER, name TEXT, parent_id INTEGER,"
                           " orientation INTEGER, file_type INTEGER, "
                           " file_date INTEGER, rating INTEGER, label INTEGER,"
                           " import_date INTEGER, mod_date INTEGER, "
                           " xmp TEXT, xmp_date INTEGER)");
    SQLStatement fsFileTable("CREATE TABLE fsfiles (id INTEGER PRIMARY KEY,"
                             " path TEXT)");
    SQLStatement keywordTable("CREATE TABLE keywords (id INTEGER PRIMARY KEY,"
                              " keyword TEXT, parent_id INTEGER)");
    SQLStatement keywordingTable("CREATE TABLE keywording (file_id INTEGER,"
                                 " keyword_id INTEGER)");
    SQLStatement xmpUpdateQueueTable("CREATE TABLE xmp_update_queue "
                                     " (id INTEGER UNIQUE)");
//		SQLStatement collsTable("CREATE TABLE collections (id INTEGER PRIMARY KEY,"
//								" name TEXT)");
//		SQLStatement collectingTable("CREATE TABLE collecting (file_id INTEGER,"
//									 " collection_id INTEGER)");

    SQLStatement fileUpdateTrigger(
        "CREATE TRIGGER file_update_trigger UPDATE ON files "
        " BEGIN"
        "  UPDATE files SET mod_date = strftime('%s','now');"
        " END");
    SQLStatement xmpUpdateTrigger(
        "CREATE TRIGGER xmp_update_trigger UPDATE OF xmp ON files "
        " BEGIN"
        "  INSERT OR IGNORE INTO xmp_update_queue (id) VALUES(new.id);"
        "  SELECT rewrite_xmp(); "
        " END");

    m_dbdrv->execute_statement(adminTable);
    m_dbdrv->execute_statement(adminVersion);
    m_dbdrv->execute_statement(vaultTable);
    m_dbdrv->execute_statement(folderTable);
    m_dbdrv->execute_statement(fileTable);
    m_dbdrv->execute_statement(fsFileTable);
    m_dbdrv->execute_statement(keywordTable);
    m_dbdrv->execute_statement(keywordingTable);
    m_dbdrv->execute_statement(xmpUpdateQueueTable);
//		m_dbdrv->execute_statement(collsTable);
//		m_dbdrv->execute_statement(collectingTable);

    m_dbdrv->execute_statement(fileUpdateTrigger);
    m_dbdrv->execute_statement(xmpUpdateTrigger);
    return true;
}

/** check that database verion
 * @return the DB version. -1 in case of error. 0 is can't read it.
 */
int Library::checkDatabaseVersion()
{
    int v = 0;
    std::string version;
    try {
        SQLStatement sql("SELECT value FROM admin WHERE key='version'");
			
        if(m_dbdrv->execute_statement(sql)) {
            if(m_dbdrv->read_next_row() 
               && m_dbdrv->get_column_content(0, version)) {
                v = boost::lexical_cast<int>(version);
            }
        }
    }
    catch(const utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
        v = -1;
    }
    catch(const boost::bad_lexical_cast &)
    {
        DBG_OUT("version is %s, can't convert to int", version.c_str());
        v = 0;
    }
    catch(...)
    {
        v = -1;
    }
    return v;
}


int Library::addFsFile(const bfs::path & file)
{
    int ret = -1;

    SQLStatement sql(boost::format("INSERT INTO fsfiles (path)"
                                   " VALUES ('%1%')") 
                     % file.string());
    if(m_dbdrv->execute_statement(sql)) {
        int64_t id = m_dbdrv->last_row_id();
        DBG_OUT("last row inserted %d", (int)id);
        ret = id;
    }
    return ret;
}


int Library::addFile(int folder_id, const bfs::path & file, bool manage)
{
    int ret = -1;
    DBG_ASSERT(!manage, "manage not supported");
    DBG_ASSERT(folder_id != -1, "invalid folder ID");
    try {
        int32_t rating, label_id, orientation;
        std::string label;  
        framework::MimeType mime = framework::MimeType(file);
        db::LibFile::FileType file_type = db::LibFile::mimetype_to_filetype(mime);
        utils::XmpMeta meta(file, file_type == db::LibFile::FILE_TYPE_RAW);
        label_id = 0;
        orientation = meta.orientation();
        rating = meta.rating();
        label = meta.label();
        time_t creation_date = meta.creation_date();
        if(creation_date == -1) {
            creation_date = 0;
        }

        int fs_file_id = addFsFile(file);
        if(fs_file_id <= 0) {
            throw(utils::Exception("add fsfile failed"));
        }
        SQLStatement sql(boost::format("INSERT INTO files ("
                                       " main_file, name, parent_id, "
                                       " import_date, mod_date, "
                                       " orientation, file_date, rating, label, file_type,"
                                       " xmp) "
                                       " VALUES ("
                                       " '%1%', '%2%', '%3%', "
                                       " '%4%', '%4%',"
                                       " '%5%', '%6%', '%7%', '%8%', '%9%',"
                                       " ?1);") 
                         % fs_file_id % file.leaf() % folder_id
                         % time(NULL)
                         % orientation % creation_date % rating
                         % folder_id % file_type);
        std::string buf = meta.serialize_inline();
        sql.bind(1, buf);
        if(m_dbdrv->execute_statement(sql)) {
            int64_t id = m_dbdrv->last_row_id();
            DBG_OUT("last row inserted %d", (int)id);
            ret = id;
            const std::vector< std::string > &keywords(meta.keywords());
            std::vector< std::string >::const_iterator iter;
            for(iter = keywords.begin();
                iter != keywords.end(); iter++) 
            {
                int kwid = makeKeyword(*iter);
                if(kwid != -1) {
                    assignKeyword(kwid, id);
                }
            }
        }
    }
    catch(const utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
        ret = -1;
    }
    catch(const std::exception & e)
    {
        DBG_OUT("unknown exception %s", e.what());
        ret = -1;
    }
    return ret;
}


int Library::addFileAndFolder(const bfs::path & folder, const bfs::path & file, 
                              bool manage)
{
    LibFolder::Ptr f;
    f = getFolder(folder);
    if(f == NULL) {
        ERR_OUT("Folder %s not found", folder.string().c_str());
    }
    return addFile(f ? f->id() : -1, file, manage);
}
	

LibFolder::Ptr Library::getFolder(const bfs::path & folder)
{
    LibFolder::Ptr f;
    SQLStatement sql(boost::format("SELECT id,name "
                                   "FROM folders WHERE path='%1%'") 
                     % folder.string());
		
    try {
        if(m_dbdrv->execute_statement(sql)) {
            if(m_dbdrv->read_next_row()) {
                int32_t id;
                std::string name;
                m_dbdrv->get_column_content(0, id);
                m_dbdrv->get_column_content(1, name);
                f = LibFolder::Ptr(new LibFolder(id, name));
            }
        }
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
    }
    return f;
}


LibFolder::Ptr Library::addFolder(const bfs::path & folder)
{
    LibFolder::Ptr f;
    SQLStatement sql(boost::format("INSERT INTO folders "
                                   "(path,name,vault_id,parent_id) "
                                   "VALUES('%1%', '%2%', '0', '0')") 
                     % folder.string() % folder.leaf());
    try {
        if(m_dbdrv->execute_statement(sql)) {
            int64_t id = m_dbdrv->last_row_id();
            DBG_OUT("last row inserted %d", (int)id);
            f = LibFolder::Ptr(new LibFolder((int)id, folder.leaf()));
        }
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
    }
    return f;
}


void Library::getAllFolders(const LibFolder::ListPtr & l)
{
    SQLStatement sql("SELECT id,name FROM folders");
    try {
        if(m_dbdrv->execute_statement(sql)) {
            while(m_dbdrv->read_next_row()) {
                int32_t id;
                std::string name;
                m_dbdrv->get_column_content(0, id);
                m_dbdrv->get_column_content(1, name);
                l->push_back(LibFolder::Ptr(new LibFolder(id, name)));
            }
        }
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
    }
}

static LibFile::Ptr getFileFromDbRow(const db::IConnectionDriver::Ptr & dbdrv)
{
    int32_t id;
    int32_t fid;
    int32_t fsfid;
    std::string pathname;
    std::string name;
    DBG_ASSERT(dbdrv->get_number_of_columns() == 9, "wrong number of columns");
    dbdrv->get_column_content(0, id);
    dbdrv->get_column_content(1, fid);
    dbdrv->get_column_content(2, pathname);
    dbdrv->get_column_content(3, name);
    dbdrv->get_column_content(8, fsfid);
    DBG_OUT("found %s", pathname.c_str());
    LibFile::Ptr f(new LibFile(id, fid, fsfid,
                               bfs::path(pathname), 
                               name));
    int32_t val;
    dbdrv->get_column_content(4, val);
    f->setOrientation(val);
    dbdrv->get_column_content(5, val);
    f->setRating(val);
    dbdrv->get_column_content(6, val);
    f->setLabel(val);

    /* Casting needed. Remember that a simple enum like this is just a couple
     * of #define for integers.
     */
    dbdrv->get_column_content(7, val);
    f->setFileType((db::LibFile::FileType)val);
    return f;
}

void Library::getFolderContent(int folder_id, const LibFile::ListPtr & fl)
{
    SQLStatement sql(boost::format("SELECT files.id,parent_id,fsfiles.path,"
                                   "name,"
                                   "orientation,rating,label,file_type,"
                                   "fsfiles.id"
                                   " FROM files,fsfiles "
                                   " WHERE parent_id='%1%' "
                                   " AND files.main_file=fsfiles.id")
                     % folder_id);
    try {
        if(m_dbdrv->execute_statement(sql)) {
            while(m_dbdrv->read_next_row()) {
                LibFile::Ptr f(getFileFromDbRow(m_dbdrv));
                fl->push_back(f);
            }
        }
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
    }
}

int Library::countFolder(int folder_id)
{
    int count = -1;
    SQLStatement sql(boost::format("SELECT COUNT(id) FROM files WHERE parent_id='%1%';")
                     % folder_id);
    try {
        if(m_dbdrv->execute_statement(sql)) {
            if(m_dbdrv->read_next_row()) {
                m_dbdrv->get_column_content(0, count);
            }
        }			
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
    }
    return count;
}

void Library::getAllKeywords(const Keyword::ListPtr & l)
{
    SQLStatement sql("SELECT id,keyword FROM keywords ORDER BY keyword");
    try {
        if(m_dbdrv->execute_statement(sql)) {
            while(m_dbdrv->read_next_row()) {
                int32_t id;
                std::string name;
                m_dbdrv->get_column_content(0, id);
                m_dbdrv->get_column_content(1, name);
                l->push_back(Keyword::Ptr(new Keyword(id, name)));
            }
        }
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
    }
}
	
int Library::makeKeyword(const std::string & keyword)
{
    int keyword_id = -1;
    SQLStatement sql("SELECT id FROM keywords WHERE "
                     "keyword=?1;");
    sql.bind(1, keyword);
    try {
        if(m_dbdrv->execute_statement(sql)) {
            if(m_dbdrv->read_next_row()) {
                m_dbdrv->get_column_content(0, keyword_id);
            }
        }
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
    }
    if(keyword_id == -1) {
        SQLStatement sql2("INSERT INTO keywords (keyword, parent_id) "
                          " VALUES(?1, 0);");
        sql2.bind(1, keyword);
        try {
            if(m_dbdrv->execute_statement(sql2)) {
                keyword_id = m_dbdrv->last_row_id();
                Keyword::Ptr kw(new Keyword(keyword_id, keyword));
                notify(NOTIFY_ADDED_KEYWORD, boost::any(kw));
            }
        }
        catch(utils::Exception & e)
        {
            DBG_OUT("db exception %s", e.what());
        }
    }

    return keyword_id;
}


bool Library::assignKeyword(int kw_id, int file_id)
{
    bool ret = false;
    SQLStatement sql(boost::format("INSERT INTO keywording (file_id, keyword_id) "
                                   " VALUES('%1%', '%2%');") 
                     % file_id % kw_id );
    try {
        ret = m_dbdrv->execute_statement(sql);
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
    }
    return ret;
}


void Library::getKeywordContent(int keyword_id, const LibFile::ListPtr & fl)
{
    SQLStatement sql(boost::format("SELECT files.id,parent_id,fsfiles.path,"
                                   "name,orientation,rating,label,file_type,"
                                   " fsfiles.id "
                                   " FROM files,fsfiles "
                                   " WHERE files.id IN "
                                   " (SELECT file_id FROM keywording "
                                   " WHERE keyword_id='%1%') "
                                   " AND fsfiles.id = files.main_file;")
                     % keyword_id);
    try {
        if(m_dbdrv->execute_statement(sql)) {
            while(m_dbdrv->read_next_row()) {
                LibFile::Ptr f(getFileFromDbRow(m_dbdrv));
                fl->push_back(f);
            }
        }
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
    }
}


void Library::getMetaData(int file_id, const LibMetadata::Ptr & meta)
{
    SQLStatement sql(boost::format("SELECT xmp FROM files "
                                   " WHERE id='%1%';")
                     % file_id);
    try {
        if(m_dbdrv->execute_statement(sql)) {
            while(m_dbdrv->read_next_row()) {
                std::string xml;
                m_dbdrv->get_column_content(0, xml);
                meta->unserialize(xml.c_str());
            }
        }
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
    }
}




bool Library::setInternalMetaDataInt(int file_id, const char* col,
                                     int32_t value)
{
    bool ret = false;
    DBG_OUT("setting metadata in column %s", col);
    SQLStatement sql(boost::format("UPDATE files SET %1%='%2%' "
                                   " WHERE id='%3%';")
                     % col % value % file_id);
    try {
        ret = m_dbdrv->execute_statement(sql);
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
        ret = false;
    }
    return ret;
}

/** set metadata block
 * @param file_id the file ID to set the metadata block
 * @param meta the metadata block
 * @return false on error
 */
bool Library::setMetaData(int file_id, const LibMetadata::Ptr & meta)
{
    bool ret = false;
    SQLStatement sql(boost::format("UPDATE files SET xmp=?1 "
                                   " WHERE id='%1%';")
                     % file_id);
    sql.bind(1, meta->serialize_inline());
    try {
        ret = m_dbdrv->execute_statement(sql);
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
        ret = false;
    }
    return ret;    
}


/** set metadata 
 * @param file_id the file ID to set the metadata block
 * @param meta the metadata index
 * @param value the value to set
 * @return false on error
 */
bool Library::setMetaData(int file_id, int meta, 
                          const boost::any & value)
{
    bool retval = false;
    DBG_OUT("setting metadata in column %x", meta);
    switch(meta) {
    case MAKE_METADATA_IDX(db::META_NS_XMPCORE, db::META_XMPCORE_RATING):
    case MAKE_METADATA_IDX(db::META_NS_XMPCORE, db::META_XMPCORE_LABEL):
    case MAKE_METADATA_IDX(db::META_NS_TIFF, db::META_TIFF_ORIENTATION):
        if(value.type() == typeid(int32_t)) {
            // internal.
            int32_t nvalue = boost::any_cast<int32_t>(value);
            // make the column mapping more generic.
            const char * col = NULL;
            switch(meta) {
            case MAKE_METADATA_IDX(db::META_NS_XMPCORE, db::META_XMPCORE_RATING):
                col = "rating";
                break;
            case MAKE_METADATA_IDX(db::META_NS_TIFF, db::META_TIFF_ORIENTATION):
                col = "orientation";
                break;
            case MAKE_METADATA_IDX(db::META_NS_XMPCORE, db::META_XMPCORE_LABEL):
                col = "label";
                break;
            }
            if(col) {
                retval = setInternalMetaDataInt(file_id, col, nvalue);
            }
        }
        break;
    default:
        // external.
        ERR_OUT("unknown metadata to set");
        return false;
    }
    LibMetadata::Ptr metablock(new LibMetadata(file_id));
    getMetaData(file_id, metablock);
    retval = metablock->setMetaData(meta, value);
    retval = metablock->touch();
    retval = setMetaData(file_id, metablock);
    return retval;
}


bool Library::getXmpIdsInQueue(std::vector<int> & ids)
{
    SQLStatement sql("SELECT id  FROM xmp_update_queue;");
    try {
        if(m_dbdrv->execute_statement(sql)) {
            while(m_dbdrv->read_next_row()) {
                int32_t id;
                m_dbdrv->get_column_content(0, id);
                ids.push_back(id);
            }
        }
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
        return false;
    }
    return true;
}


bool Library::rewriteXmpForId(int id)
{
    SQLStatement del(boost::format("DELETE FROM xmp_update_queue "
                                   " WHERE id='%1%';") % id);
    SQLStatement getxmp(boost::format("SELECT xmp, path FROM files "
                                   " WHERE id='%1%';") % id);
    try {
        
        if(m_dbdrv->execute_statement(del) 
           && m_dbdrv->execute_statement(getxmp)) {
            while(m_dbdrv->read_next_row()) {
                std::string xmp_buffer;
                std::string spath;
                m_dbdrv->get_column_content(0, xmp_buffer);
                m_dbdrv->get_column_content(1, spath);
                boost::filesystem::path p;
                p = boost::filesystem::change_extension(spath, ".xmp");
                DBG_ASSERT(p.string() != spath, "path must have been changed");
                if(exists(p)) {
                    DBG_OUT("%s already exist", p.string().c_str());
                    // TODO backup
                }
                // TODO probably a faster way to do that
                utils::XmpMeta xmppacket;
                xmppacket.unserialize(xmp_buffer.c_str());
                // TODO use different API
                FILE * f = fopen(p.string().c_str(), "w");
                if(f) {
                    std::string sidecar = xmppacket.serialize();
                    fwrite(sidecar.c_str(), sizeof(std::string::value_type),
                           sidecar.size(), f);
                    fclose(f);
                }
                // TODO rewrite the modified date in the files table
                // caveat: this will trigger this rewrite recursively.
            }
        }
    }
    catch(utils::Exception & e)
    {
        DBG_OUT("db exception %s", e.what());
        return false;
    }    
    return true;
}


bool Library::processXmpUpdateQueue()
{
    bool retval = false;
    std::vector<int> ids;
    retval = getXmpIdsInQueue(ids);
    if(retval) {
        std::for_each(ids.begin(), ids.end(),
                     boost::bind(&Library::rewriteXmpForId,
                                 this, _1));
    }
    return retval;
}


}
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/