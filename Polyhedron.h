﻿//
// Created by yrz on 7/7/22.
//

#ifndef POLYHEDRON_H
#define POLYHEDRON_H
#include <memory>
#include <utility>
#include <type_traits>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <CGAL/Bbox_3.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/IO/Color.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <CGAL/Polyhedron_items_with_id_3.h>
#include <nlohmann/json.hpp>

template <typename SizeType>
struct TTriangle
{
public:
    using size_type = SizeType;
    TTriangle(SizeType i0, SizeType i1, SizeType i2)
    {
        _id[0] = i0;
        _id[1] = i1;
        _id[2] = i2;
    }
    SizeType &operator[](uint32_t i) { return _id[i]; }
    const SizeType &operator[](uint32_t i) const { return _id[i]; }
    std::pair<SizeType, SizeType> GetEdge(uint32_t i)
    {
        switch (i)
        {
        case 0:
            return std::make_pair(_id[0], _id[1]);
            break;
        case 1:
            return std::make_pair(_id[1], _id[2]);
            break;
        case 2:
            return std::make_pair(_id[2], _id[0]);
            break;
        }
        return std::make_pair<size_t, size_t>(0, 0);
    }

protected:
    SizeType _id[3]{0, 0, 0};
};

template <typename SizeType>
struct TEdge
{
public:
    using size_type = SizeType;

    TEdge() = default;

    TEdge(SizeType i0, SizeType i1)
    {
        _i0 = i0;
        _i1 = i1;
    }

    SizeType _i0;
    SizeType _i1;
    std::vector<SizeType> _faces;
};

template <typename SizeType>
struct TPairHashUnordered
{
    using size_type = SizeType;
    size_t operator()(const std::pair<SizeType, SizeType> &p) const
    {
        if (p.first > p.second)
        {
            return boost::hash<std::pair<SizeType, SizeType>>()(p);
        }
        else
        {
            return boost::hash<std::pair<SizeType, SizeType>>()({p.second, p.first});
        }
    }
};

template <typename SizeType>
struct TPairPredUnordered
{
    using size_type = SizeType;

    bool operator()(std::pair<SizeType, SizeType> e0, std::pair<SizeType, SizeType> e1) const
    {
        return e0.first == e1.first && e0.second == e1.second || e0.first == e1.second && e0.second == e1.first;
    }
};

template <typename SizeType>
struct TPairHash
{
    using size_type = SizeType;
    size_t operator()(std::pair<SizeType, SizeType> edge) const
    {
        return boost::hash<std::pair<SizeType, SizeType>>()(edge);
    }
};

template <typename SizeType>
struct TPairPred
{
    using size_type = SizeType;
    bool operator()(std::pair<SizeType, SizeType> edge0, std::pair<SizeType, SizeType> edge1) const
    {
        return edge0.first == edge1.first && edge0.second == edge1.second;
    }
};
template <typename HDS, typename Kernel>
class TPolyhedronObjBulider;

template <typename Item, typename Kernel>
#if BOOST_CXX_VERSION >= 202002L
    requires std::derived_from<Item, CGAL::Polyhedron_items_with_id_3>
#endif
class TPolyhedron : public CGAL::Polyhedron_3<Kernel, Item>
{
public:
    static_assert(std::is_base_of_v<CGAL::Polyhedron_items_with_id_3, Item>, "Item has to derive from Polyhedron_items_with_id_3!");
    using Base = CGAL::Polyhedron_3<Kernel, Item>;
    using PolyhedronObjBuilder = TPolyhedronObjBulider<typename Base::HalfedgeDS, Kernel>;
    using Triangle = TTriangle<typename Base::Vertex::size_type>;
    using Edge = TEdge<typename Base::Vertex::size_type>;
    using PairHashUnordered = TPairHashUnordered<typename Base::Vertex::size_type>;
    using PairPredUnordered = TPairPredUnordered<typename Base::Vertex::size_type>;
    using PairHash = TPairHash<typename Base::Vertex::size_type>;
    using PairPred = TPairPred<typename Base::Vertex::size_type>;

    TPolyhedron(const std::vector<typename Kernel::Point_3> &vertices, const std::vector<size_t> &indices)
    {
        PolyhedronObjBuilder builder(vertices, indices);
        this->delegate(builder);
        CGAL::set_halfedgeds_items_id(*this);
    }

    TPolyhedron() = default;

    std::pair<std::vector<typename Kernel::Point_3>, std::vector<size_t>> ToVerticesIndices() const
    {
        CGAL::set_halfedgeds_items_id(const_cast<TPolyhedron<Item, Kernel> &>(*this));
        std::vector<typename Kernel::Point_3> vertices;
        std::vector<size_t> indices;
        for (auto hv = this->vertices_begin(); hv != this->vertices_end(); hv++)
        {
            vertices.push_back(hv->point());
        }
        for (auto hf = this->facets_begin(); hf != this->facets_end(); hf++)
        {
            indices.push_back(hf->halfedge()->vertex()->id());
            indices.push_back(hf->halfedge()->next()->vertex()->id());
            indices.push_back(hf->halfedge()->prev()->vertex()->id());
        }
        return {vertices, indices};
    }

    std::pair<std::vector<typename Kernel::Point_3>, std::vector<Triangle>> ToVerticesTriangles() const
    {
        CGAL::set_halfedgeds_items_id(const_cast<TPolyhedron<Item, Kernel> &>(*this));
        std::vector<typename Kernel::Point_3> vertices;
        std::vector<Triangle> triangles;
        for (auto hv = this->vertices_begin(); hv != this->vertices_end(); hv++)
        {
            vertices.push_back(hv->point());
        }
        for (auto hf = this->facets_begin(); hf != this->facets_end(); hf++)
        {
            triangles.emplace_back(
                hf->halfedge()->vertex()->id(),
                hf->halfedge()->next()->vertex()->id(),
                hf->halfedge()->prev()->vertex()->id());
        }
        return {vertices, triangles};
    }

    void WriteOFF(const std::string &path) const
    {
        std::ofstream ofs(path);
        CGAL::IO::write_OFF(ofs, *this);
    }

    void WriteOBJ(const std::string &path)
    {
        CGAL::set_halfedgeds_items_id(*this);
        std::stringstream ss;
        for (auto hv = this->vertices_begin(); hv != this->vertices_end(); hv++)
        {
            ss << "v " << hv->point().x() << ' ' << hv->point().y() << ' ' << hv->point().z() << '\n';
        }
        for (auto hf = this->facets_begin(); hf != this->facets_end(); hf++)
        {
            size_t e0 = hf->halfedge()->id();
            size_t e1 = hf->halfedge()->next()->id();
            size_t e2 = hf->halfedge()->prev()->id();
            size_t v0 = hf->halfedge()->vertex()->id();
            size_t v1 = hf->halfedge()->next()->vertex()->id();
            size_t v2 = hf->halfedge()->prev()->vertex()->id();
            ss << "f " << v0 << "//" << e0 << ' ' << v1 << "//" << e1 << ' ' << v2 << "//" << e2 << '\n';
        }
        std::ofstream ofs(path);
        ofs << ss.rdbuf();
        ofs.close();
    }

    virtual bool WriteAssimp( const std::string& path) const
    {
        Assimp::Exporter exporter;
        auto scene = std::make_unique<aiScene>();

        scene->mRootNode = new aiNode();
        scene->mRootNode->mNumMeshes = 1;
        scene->mRootNode->mMeshes = new uint32_t[1];
        scene->mRootNode->mMeshes[0] = 0;

        scene->mNumMaterials = 1;
        scene->mMaterials = new aiMaterial*[]{ new aiMaterial() };
        scene->mMetaData = new aiMetadata();

        scene->mNumMeshes = 1;
        scene->mMeshes = new aiMesh*[1];
        scene->mMeshes[0] = new aiMesh();

        aiMesh* m = scene->mMeshes[0];
        m->mNumFaces = static_cast<uint32_t>(this->size_of_facets());
        m->mNumVertices = static_cast<uint32_t>(this->size_of_vertices());
        m->mVertices = new aiVector3D[m->mNumVertices];
        m->mFaces = new aiFace[m->mNumFaces];
        m->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

        auto hv = this->vertices_begin();
        for(size_t i = 0; i < m->mNumVertices; i++, hv++)
        {
            m->mVertices[i] = aiVector3D(
                static_cast<ai_real>(hv->point().x()),
                static_cast<ai_real>(hv->point().y()),
                static_cast<ai_real>(hv->point().z()));
        }

        auto hf = this->facets_begin();
        for(size_t i = 0; i < m->mNumFaces; i++, hf++)
        {
            m->mFaces[i].mNumIndices = 3;
            m->mFaces[i].mIndices = new unsigned int[3];
            m->mFaces[i].mIndices[0] = static_cast<unsigned int>(hf->halfedge()->vertex()->id());
            m->mFaces[i].mIndices[1] = static_cast<unsigned int>(hf->halfedge()->next()->vertex()->id());
            m->mFaces[i].mIndices[2] = static_cast<unsigned int>(hf->halfedge()->prev()->prev()->vertex()->id());
        }

        std::string postfix = path.substr(path.rfind('.') + 1);
        
        if(postfix == std::string("ply"))
        {
            postfix = "plyb";
        }
        else if (postfix == std::string("stl"))
        {
            postfix = "stlb";
        }
        //Assimp::ExportProperties prop;
        return exporter.Export(scene.get(), postfix, path) == aiReturn_SUCCESS;
    }

    bool IsSmallHole(typename Base::Halfedge_handle hh, int max_num_hole_edges, float max_hole_diam)
    {
        int num_hole_edges = 0;
        CGAL::Bbox_3 hole_bbox;
        for (typename Base::Halfedge_handle hc : CGAL::halfedges_around_face(hh, *this))
        {
            const typename Kernel::Point_3& p = hc->vertex()->point();
            hole_bbox += p.bbox();
            ++num_hole_edges;
            // Exit early, to avoid unnecessary traversal of large holes
            if (num_hole_edges > max_num_hole_edges) return false;
            if (hole_bbox.xmax() - hole_bbox.xmin() > max_hole_diam) return false;
            if (hole_bbox.ymax() - hole_bbox.ymin() > max_hole_diam) return false;
            if (hole_bbox.zmax() - hole_bbox.zmin() > max_hole_diam) return false;
        }
        return true;
    }
};

template <class Refs, typename Tag, typename Point>
class VertexWithLabelFlag : public CGAL::HalfedgeDS_vertex_max_base_with_id<Refs, Point, size_t>
{
public:
    VertexWithLabelFlag() = default;
    explicit VertexWithLabelFlag(const Point &p) : CGAL::HalfedgeDS_vertex_max_base_with_id<Refs, Point, size_t>(p) {}
    VertexWithLabelFlag(const VertexWithLabelFlag &p) : CGAL::HalfedgeDS_vertex_max_base_with_id<Refs, Point, size_t>(p), _label(p._label) {}
    VertexWithLabelFlag(const Point &p, int label) : CGAL::HalfedgeDS_vertex_max_base_with_id<Refs, Point, size_t>(p), _label(label) {}

public:
    int _label{0};
    bool _processed{false};
};

template <class Refs>
class FaceWithLabelFlag : public CGAL::HalfedgeDS_face_max_base_with_id<Refs, CGAL::Tag_true, size_t>
{
public:
    int _label{0};
    bool _processed{false};
};

class ItemsWithLabelFlag : public CGAL::Polyhedron_items_with_id_3
{
public:
    template <class Refs, class Traits>
    struct Vertex_wrapper
    {
        using Point = typename Traits::Point_3;
        using Vertex = VertexWithLabelFlag<Refs, CGAL::Tag_true, Point>;
    };

    template <class Refs, class Traits>
    struct Face_wrapper
    {
        using Face = FaceWithLabelFlag<Refs>;
    };
};

template <typename Item, typename Kernel>
#if BOOST_CXX_VERSION >= 202002L
    requires std::derived_from<Item, ItemsWithLabelFlag>
#endif
class TPolyhedronWithLabel : public TPolyhedron<Item, Kernel>
{
public:
    static_assert(std::is_base_of_v<ItemsWithLabelFlag, Item>, "Item has to derive from ItemsWithLabelFlag!");
    using Base = TPolyhedron<Item, Kernel>;

    TPolyhedronWithLabel(const std::vector<typename Kernel::Point_3> &vertices, const std::vector<size_t> &indices)
        :Base(vertices, indices)
    {
    }

    TPolyhedronWithLabel()
        :Base()
    {}

    bool LoadLabels( const std::string& path )
    {
        using namespace nlohmann;
        CGAL::set_halfedgeds_items_id(*this);

        std::ifstream label_ifs( path );
        json data = json::parse( label_ifs );
        if (data.find( "labels" ) == data.end())
        {
            std::cout << "Invalid Json" << std::endl;
            return false;
        }
        std::vector<int> labels = data["labels"].get<std::vector<int>>();
        if(labels.size() != this->size_of_vertices())
        {
            std::cout << "number of labels != number of vertices" << std::endl;
            return false;
        }
        
        for(auto hv = this->vertices_begin(); hv != this->vertices_end(); hv++)
        {
            hv->_label = labels[hv->id()];
            if(hv->_label == 100 || hv->_label < 10)
                hv->_label = 0;
        }

        for(auto hf : CGAL::faces(*this))
        {
            int l0 = hf->halfedge()->vertex()->_label;
            int l1 = hf->halfedge()->next()->vertex()->_label;
            int l2 = hf->halfedge()->prev()->vertex()->_label;
            hf->_label = std::max(l0, std::max(l1, l2));
        }

        return true;
    }

    bool WriteLabels( const std::string& path ) const
    {
        using namespace nlohmann;
        std::vector<int> labels;
        for (auto hv : CGAL::vertices(*this))
        {
            labels.push_back(hv->_label);
        }
        json j;
        j["labels"] = labels;

        std::ofstream ofs(path);
        ofs << j;
        ofs.close();
        return !ofs.fail();
    }

    bool WriteLabels( const std::string& path, const std::string& ori_path ) const
    {
        using namespace nlohmann;
        std::vector<int> labels;
        for (auto hv : CGAL::vertices(*this))
        {
            labels.push_back(hv->_label);
        }

        std::ifstream ori_ifs(ori_path);
        json ori_json = json::parse(ori_ifs);
        ori_ifs.close();

        ori_json["labels"] = labels;
        std::ofstream ofs(path);
        ofs << ori_json;
        ofs.close();
        return !ofs.fail();
    }

    virtual bool WriteAssimp( const std::string& path) const override
    {
        static const std::array<aiColor4D, 10> COLORS = {
            aiColor4D{142.0f / 255, 207.0f / 255, 201.0f / 255, 1.0},
            aiColor4D{255.0f / 255, 190.0f / 255, 122.0f / 255, 1.0},
            aiColor4D{250.0f / 255, 127.0f / 255, 111.0f / 255, 1.0},
            aiColor4D{130.0f / 255, 176.0f / 255, 210.0f / 255, 1.0},
            aiColor4D{190.0f / 255, 184.0f / 255, 220.0f / 255, 1.0},
            aiColor4D{40.0f / 255, 120.0f / 255, 181.0f / 255, 1.0},
            aiColor4D{248.0f / 255, 172.0f / 255, 140.0f / 255, 1.0},
            aiColor4D{255.0f / 255, 136.0f / 255, 132.0f / 255, 1.0},
            aiColor4D{84.0f / 255, 179.0f / 255, 69.0f / 255, 1.0},
            aiColor4D{137.0f / 255, 131.0f / 255, 191.0f / 255, 1.0}
        };
        
        Assimp::Exporter exporter;
        auto scene = std::make_unique<aiScene>();

        scene->mRootNode = new aiNode();
        scene->mRootNode->mNumMeshes = 1;
        scene->mRootNode->mMeshes = new uint32_t[1];
        scene->mRootNode->mMeshes[0] = 0;

        scene->mNumMaterials = 1;
        scene->mMaterials = new aiMaterial*[]{ new aiMaterial() };
        scene->mMetaData = new aiMetadata();

        scene->mNumMeshes = 1;
        scene->mMeshes = new aiMesh*[1];
        scene->mMeshes[0] = new aiMesh();

        aiMesh* m = scene->mMeshes[0];
        m->mNumFaces = static_cast<uint32_t>(this->size_of_facets());
        m->mNumVertices = static_cast<uint32_t>(this->size_of_vertices());
        m->mVertices = new aiVector3D[m->mNumVertices];
        m->mColors[0] = new aiColor4D[m->mNumVertices];
        m->mFaces = new aiFace[m->mNumFaces];
        m->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

        auto hv = this->vertices_begin();
        for(size_t i = 0; i < m->mNumVertices; i++, hv++)
        {
            m->mVertices[i] = aiVector3D(
                static_cast<ai_real>(hv->point().x()),
                static_cast<ai_real>(hv->point().y()),
                static_cast<ai_real>(hv->point().z()));
            m->mColors[0][i] = COLORS[hv->_label % COLORS.size()];
        }

        auto hf = this->facets_begin();
        for(size_t i = 0; i < m->mNumFaces; i++, hf++)
        {
            m->mFaces[i].mNumIndices = 3;
            m->mFaces[i].mIndices = new unsigned int[3];
            m->mFaces[i].mIndices[0] = static_cast<unsigned int>(hf->halfedge()->vertex()->id());
            m->mFaces[i].mIndices[1] = static_cast<unsigned int>(hf->halfedge()->next()->vertex()->id());
            m->mFaces[i].mIndices[2] = static_cast<unsigned int>(hf->halfedge()->prev()->prev()->vertex()->id());
        }

        std::string postfix = path.substr(path.rfind('.') + 1);
        
        if(postfix == std::string("ply"))
        {
            postfix = "plyb";
        }
        else if (postfix == std::string("stl"))
        {
            postfix = "stlb";
        }
        //Assimp::ExportProperties prop;
        return exporter.Export(scene.get(), postfix, path) == aiReturn_SUCCESS;
    }
};

#define CGAL_GRAPH_TRAITS_INHERITANCE_TEMPLATE_PARAMS typename Item, typename Kernel
#define CGAL_GRAPH_TRAITS_INHERITANCE_CLASS_NAME TPolyhedron<Item, Kernel>
#define CGAL_GRAPH_TRAITS_INHERITANCE_BASE_CLASS_NAME CGAL::Polyhedron_3<Kernel, Item>
#include <CGAL/boost/graph/graph_traits_inheritance_macros.h>
#define CGAL_GRAPH_TRAITS_INHERITANCE_TEMPLATE_PARAMS typename Item, typename Kernel
#define CGAL_GRAPH_TRAITS_INHERITANCE_CLASS_NAME TPolyhedronWithLabel<Item, Kernel>
#define CGAL_GRAPH_TRAITS_INHERITANCE_BASE_CLASS_NAME CGAL::Polyhedron_3<Kernel, Item>
#include <CGAL/boost/graph/graph_traits_inheritance_macros.h>
#undef CGAL_GRAPH_TRAITS_INHERITANCE_CLASS_NAME
#undef CGAL_GRAPH_TRAITS_INHERITANCE_BASE_CLASS_NAME

template <typename HDS, typename Kernel>
class TPolyhedronObjBulider : public CGAL::Modifier_base<HDS>
{
public:
    TPolyhedronObjBulider(const std::vector<typename Kernel::Point_3> &vertices, const std::vector<size_t> &indices)
        : _vertices(vertices), _indices(indices) {}
    virtual void operator()(HDS &hds) override
    {
        CGAL::Polyhedron_incremental_builder_3<HDS> builder(hds, true);
        builder.begin_surface(_vertices.size(), _indices.size() / 3);
        for (size_t i = 0, size = _vertices.size(); i < size; i += 1)
        {
            builder.add_vertex(_vertices[i]);
        }
        for (size_t f = 0, size = _indices.size() / 3; f < size; ++f)
        {
            builder.begin_facet();
            builder.add_vertex_to_facet(_indices[f * 3 + 0]);
            builder.add_vertex_to_facet(_indices[f * 3 + 1]);
            builder.add_vertex_to_facet(_indices[f * 3 + 2]);
            typename HDS::Halfedge_handle hh = builder.end_facet();
            if (hh == nullptr)
            {
                std::cout << "Err: face " << f << ':' << _indices[f * 3 + 0] << ',' << _indices[f * 3 + 1] << ',' << _indices[f * 3 + 2] << std::endl;
            }
        }
        builder.end_surface();
    }

protected:
    const std::vector<typename Kernel::Point_3> &_vertices;
    const std::vector<size_t> &_indices;
};

template <typename Kernel, typename SizeType>
bool LoadVFAssimp( const std::string& path, std::vector<typename Kernel::Point_3>& vertices, std::vector<TTriangle<SizeType>>& faces )
{
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
     aiComponent_ANIMATIONS | aiComponent_COLORS | aiComponent_NORMALS | aiComponent_TEXCOORDS | aiComponent_TANGENTS_AND_BITANGENTS );
    const aiScene* scene = importer.ReadFile(path, aiProcess_JoinIdenticalVertices | aiProcess_RemoveComponent);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || scene->mNumMeshes < 1)
    {
        printf_s("Error: LoadVFAssimp: cannot read mesh.\n");
        return false;
    }
    const aiMesh* mesh = scene->mMeshes[0];

    auto to_cgal = [](const aiVector3D& v) { return typename Kernel::Point_3{v.x, v.y, v.z};};

    vertices.clear();
    vertices.reserve(mesh->mNumVertices);
    for(uint32_t i = 0; i < mesh->mNumVertices; i++)
    {
        vertices.push_back(to_cgal(mesh->mVertices[i]));
    }

    faces.clear();
    faces.reserve(mesh->mNumVertices);
    for(uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        const auto& f = mesh->mFaces[i];
        if(f.mIndices[0] >= mesh->mNumVertices || f.mIndices[1] >= mesh->mNumVertices || f.mIndices[2] >= mesh->mNumVertices)
        {
            printf_s("Error: LoadVFAssimp: found bad index.\n");
            return false;
        }
        faces.emplace_back(f.mIndices[0], f.mIndices[1], f.mIndices[2]);
    }

    return true;
}

template <typename Kernel, typename SizeType>
bool WriteVFAssimp( std::string path, const std::vector<typename Kernel::Point_3>& vertices, const std::vector<TTriangle<SizeType>>& faces, const std::vector<int>& labels)
{
     static const std::array<aiColor4D, 10> COLORS = {
            aiColor4D{142.0f / 255, 207.0f / 255, 201.0f / 255, 1.0},
            aiColor4D{255.0f / 255, 190.0f / 255, 122.0f / 255, 1.0},
            aiColor4D{250.0f / 255, 127.0f / 255, 111.0f / 255, 1.0},
            aiColor4D{130.0f / 255, 176.0f / 255, 210.0f / 255, 1.0},
            aiColor4D{190.0f / 255, 184.0f / 255, 220.0f / 255, 1.0},
            aiColor4D{40.0f / 255, 120.0f / 255, 181.0f / 255, 1.0},
            aiColor4D{248.0f / 255, 172.0f / 255, 140.0f / 255, 1.0},
            aiColor4D{255.0f / 255, 136.0f / 255, 132.0f / 255, 1.0},
            aiColor4D{84.0f / 255, 179.0f / 255, 69.0f / 255, 1.0},
            aiColor4D{137.0f / 255, 131.0f / 255, 191.0f / 255, 1.0}
        };
        
        Assimp::Exporter exporter;
        auto scene = std::make_unique<aiScene>();

        scene->mRootNode = new aiNode();
        scene->mRootNode->mNumMeshes = 1;
        scene->mRootNode->mMeshes = new uint32_t[1];
        scene->mRootNode->mMeshes[0] = 0;

        scene->mNumMaterials = 1;
        scene->mMaterials = new aiMaterial*[]{ new aiMaterial() };
        scene->mMetaData = new aiMetadata();

        scene->mNumMeshes = 1;
        scene->mMeshes = new aiMesh*[1];
        scene->mMeshes[0] = new aiMesh();

        aiMesh* m = scene->mMeshes[0];
        m->mNumFaces = static_cast<uint32_t>(faces.size());
        m->mNumVertices = static_cast<uint32_t>(vertices.size());
        m->mVertices = new aiVector3D[m->mNumVertices];
        m->mColors[0] = new aiColor4D[m->mNumVertices];
        m->mFaces = new aiFace[m->mNumFaces];
        m->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

        for(size_t i = 0; i < m->mNumVertices; i++)
        {
            m->mVertices[i] = aiVector3D(
                static_cast<ai_real>(vertices[i].x()),
                static_cast<ai_real>(vertices[i].y()),
                static_cast<ai_real>(vertices[i].z()));
            m->mColors[0][i] = COLORS[labels[i] % COLORS.size()];
        }

        for(size_t i = 0; i < m->mNumFaces; i++)
        {
            m->mFaces[i].mNumIndices = 3;
            m->mFaces[i].mIndices = new unsigned int[m->mFaces[i].mNumIndices];
            m->mFaces[i].mIndices[0] = static_cast<unsigned int>(faces[i][0]);
            m->mFaces[i].mIndices[1] = static_cast<unsigned int>(faces[i][1]);
            m->mFaces[i].mIndices[2] = static_cast<unsigned int>(faces[i][2]);
        }

        std::string postfix = path.substr(path.rfind('.') + 1);
        
        if(postfix == std::string("ply"))
        {
            postfix = "plyb";
        }
        else if (postfix == std::string("stl"))
        {
            postfix = "stlb";
        }
        //Assimp::ExportProperties prop;
        return exporter.Export(scene.get(), postfix, path) == aiReturn_SUCCESS;
}

template <typename Facet_handle>
Facet_handle::value_type::Vertex::Point_3::R::Vector_3 FaceNormal(Facet_handle hf)
{
    auto p0 = hf->halfedge()->vertex()->point();
    auto p1 = hf->halfedge()->next()->vertex()->point();
    auto p2 = hf->halfedge()->prev()->vertex()->point();
    auto v1 = p1 - p0;
    auto v2 = p2 - p0;
    auto n = CGAL::cross_product(v1, v2);
    n /= std::sqrt(n.squared_length());
    return n;
}

#endif // POLYHEDRON_H