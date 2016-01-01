/*
 * niepce - engine/importer/directoryimporter.hpp
 *
 * Copyright (C) 2014-2015 Hubert Figuière
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


#pragma once

#include <mutex>

#include "fwk/utils/files.hpp"
#include "engine/importer/iimporter.hpp"

namespace eng {


class DirectoryImporter
  : public IImporter {
public:
  DirectoryImporter();
  virtual ~DirectoryImporter();

  std::string name() const override;

  bool listSourceContent(const std::string & source) override;
  std::list<ImportedFile::Ptr> getSourceContent() override;
  bool doImport(const std::string & source,
                const file_importer & importer) override;
private:
  std::mutex m_content_lock;
  std::list<ImportedFile::Ptr> m_content;
};

}