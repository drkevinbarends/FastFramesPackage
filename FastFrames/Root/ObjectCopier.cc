/**
 * @file ObjectCopier.cc
 * @brief Class responsible for copying metedata from input ntuple to output ntuple
 *
 */

#include "FastFrames/ObjectCopier.h"

#include "FastFrames/Logger.h"

#include "ROOT/RDataFrame.hxx"
#include "TClass.h"
#include "TEfficiency.h"
#include "TFile.h"
#include "TH1.h"
#include "TKey.h"
#include "TNamed.h"
#include "TROOT.h"

#include <exception>

ObjectCopier::ObjectCopier(const std::vector<std::string>& fileList) noexcept :
m_fileList(fileList)
{
}

void ObjectCopier::readObjectInfo() {
    if (m_fileList.empty()) {
        LOG(ERROR) << "File list cannot be empty!\n";
        throw std::invalid_argument("");
    }
    std::unique_ptr<TFile> in(TFile::Open(m_fileList.at(0).c_str(), "READ"));
    if (!in) {
        LOG(ERROR) << "Cannot open file at: " << m_fileList.at(0) << "\n";
        throw std::invalid_argument("");
    }

    TKey* key;
    TIter nextkey(in->GetListOfKeys());
    while ((key = static_cast<TKey*>(nextkey()))) {
        const std::string classname = key->GetClassName();
        const TClass *cl = gROOT->GetClass(classname.c_str());
        if (!cl) continue;
        if (cl->InheritsFrom("TTree")) {
            m_objectList.emplace_back(std::make_pair(key->GetName(), ObjectCopier::ObjectType::Tree));
            continue;
        }
        if (cl->InheritsFrom("TH1")) {
            m_objectList.emplace_back(std::make_pair(key->GetName(), ObjectCopier::ObjectType::Histogram));
            continue;
        }
        if (cl->InheritsFrom("TEfficiency")) {
            m_objectList.emplace_back(std::make_pair(key->GetName(), ObjectCopier::ObjectType::Efficiency));
            continue;
        }
    }
}

void ObjectCopier::copyObjectsTo(const std::string& outputPath) const {
    std::vector<std::unique_ptr<TFile> > files;
    for (const auto& ifile : m_fileList) {
        std::unique_ptr<TFile> f(TFile::Open(ifile.c_str(), "READ"));
        if (!f) {
            LOG(ERROR) << "Cannot open file at: " << ifile << "\n";
            throw std::invalid_argument("");
        }
        files.emplace_back(std::move(f));
    }

    std::unique_ptr<TFile> out(TFile::Open(outputPath.c_str(), "Update"));
    if (!out) {
        LOG(ERROR) << "Unable to open file at: " << outputPath << "\n";
        throw std::invalid_argument("");
    }

    for (const auto& iobject : m_objectList) {
        if (iobject.second == ObjectCopier::ObjectType::Histogram) {
            auto finalHist = this->mergeHistos(iobject.first, files);
            out->cd();
            finalHist->Write(iobject.first.c_str());
        }
        if (iobject.second == ObjectCopier::ObjectType::Efficiency) {
            auto finalHist = this->mergeEfficiencies(iobject.first, files);
            out->cd();
            finalHist->Write(iobject.first.c_str());
        }
    }
}

std::unique_ptr<TH1> ObjectCopier::mergeHistos(const std::string& name,
                                               const std::vector<std::unique_ptr<TFile> >& files) const {

    std::unique_ptr<TH1> result(nullptr);

    for (const auto& ifile : files) {
        std::unique_ptr<TH1> hist(ifile->Get<TH1>(name.c_str()));
        if (!hist) {
            LOG(ERROR) << "Cannot read histogram: " << name << "\n";
            throw std::invalid_argument("");
        }
        hist->SetDirectory(nullptr);

        if (result) {
            result->Add(hist.get());
        } else {
            result = std::move(hist);
        }
    }

    return result;
}

std::unique_ptr<TEfficiency> ObjectCopier::mergeEfficiencies(const std::string& name,
                                                             const std::vector<std::unique_ptr<TFile> >& files) const {

    std::unique_ptr<TEfficiency> result(nullptr);

    for (const auto& ifile : files) {
        std::unique_ptr<TEfficiency> hist(ifile->Get<TEfficiency>(name.c_str()));
        if (!hist) {
            LOG(ERROR) << "Cannot read TEfficiency: " << name << "\n";
            throw std::invalid_argument("");
        }
        hist->SetDirectory(nullptr);

        if (result) {
            *result += *hist;
        } else {
            result = std::move(hist);
        }
    }

    return result;
}

void ObjectCopier::copyTreesTo(const std::string& outputPath,
                               const std::vector<std::string>& trees,
                               const bool convertVecToRVec) const {

    // Use RDF to merge trees and store them
    for (const auto& iobject : m_objectList) {
        if (iobject.second != ObjectType::Tree) continue;
        const std::string& name = iobject.first;
        auto itr = std::find(trees.begin(), trees.end(), name);
        if (itr == trees.end()) continue;

        LOG(INFO) << "Started copying tree: " << name << " to " << outputPath << "\n";
        ROOT::RDF::RSnapshotOptions opts;
        opts.fMode = "UPDATE";
        opts.fVector2RVec = convertVecToRVec;
        ROOT::RDataFrame df(name, m_fileList);
        df.Snapshot(name, outputPath, df.GetColumnNames(), opts);
        LOG(INFO) << "Finished copying tree: " << name << " to " << outputPath << "\n";
    }
}