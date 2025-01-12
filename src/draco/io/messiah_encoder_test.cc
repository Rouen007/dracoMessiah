// Copyright 2017 The Draco Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "draco/io/messiah_encoder.h"

#include <sstream>

#include "draco/attributes/geometry_attribute.h"
#include "draco/core/draco_test_base.h"
#include "draco/core/draco_test_utils.h"
#include "draco/io/file_reader_factory.h"
#include "draco/io/file_reader_interface.h"
#include "draco/io/file_utils.h"
#include "draco/io/messiah_decoder.h"

namespace draco {

class MessiahEncoderTest : public ::testing::Test {
 protected:
  void CompareMeshes(const Mesh *mesh0, const Mesh *mesh1) {
    ASSERT_NE(mesh0, nullptr);
    ASSERT_NE(mesh1, nullptr);
    ASSERT_EQ(mesh0->num_faces(), mesh1->num_faces());
    ASSERT_EQ(mesh0->num_attributes(), mesh1->num_attributes());
    for (size_t att_id = 0; att_id < mesh0->num_attributes(); ++att_id) {
      ASSERT_EQ(mesh0->attribute(att_id)->size(),
                mesh1->attribute(att_id)->size());
    }
  }

  // Encode a mesh using the MessiahEncoder and then decode to verify the encoding.
  std::unique_ptr<Mesh> EncodeAndDecodeMesh(const Mesh *mesh) {
    EncoderBuffer encoder_buffer;
    MessiahEncoder encoder;
    if (!encoder.EncodeToBuffer(*mesh, &encoder_buffer)) {
      return nullptr;
    }

    DecoderBuffer decoder_buffer;
    decoder_buffer.Init(encoder_buffer.data(), encoder_buffer.size());
    std::unique_ptr<Mesh> decoded_mesh(new Mesh());
    ObjDecoder decoder;
    decoder.set_use_metadata(true);
    if (!decoder.DecodeFromBuffer(&decoder_buffer, decoded_mesh.get()).ok()) {
      return nullptr;
    }
    return decoded_mesh;
  }

  void test_encoding(const std::string &file_name) {
    const std::unique_ptr<Mesh> mesh(ReadMeshFromTestFile(file_name, true));

    ASSERT_NE(mesh, nullptr) << "Failed to load test model " << file_name;
    ASSERT_GT(mesh->num_faces(), 0);

    const std::unique_ptr<Mesh> decoded_mesh = EncodeAndDecodeMesh(mesh.get());
    CompareMeshes(mesh.get(), decoded_mesh.get());
  }
};

TEST_F(MessiahEncoderTest, HasSubObject) { test_encoding("cube_att_sub_o.obj"); }

TEST_F(MessiahEncoderTest, HasMaterial) {
  const std::unique_ptr<Mesh> mesh0(ReadMeshFromTestFile("mat_test.obj", true));
  ASSERT_NE(mesh0, nullptr);
  const std::unique_ptr<Mesh> mesh1 = EncodeAndDecodeMesh(mesh0.get());
  ASSERT_NE(mesh1, nullptr);
  ASSERT_EQ(mesh0->num_faces(), mesh1->num_faces());
  ASSERT_EQ(mesh0->num_attributes(), mesh1->num_attributes());
  // Position attribute should be the same.
  ASSERT_EQ(mesh0->attribute(0)->size(), mesh1->attribute(0)->size());
  // Since |mesh1| is decoded from buffer, it has not material file. So the
  // size of material attribute is the number of materials used in the obj
  // file which is 7. The size of material attribute of |mesh0| decoded from
  // the obj file will be the number of materials defined in the .mtl file.
  ASSERT_EQ(mesh0->attribute(1)->size(), 29);
  ASSERT_EQ(mesh1->attribute(1)->size(), 7);
}

TEST_F(MessiahEncoderTest, TestObjEncodingAll) {
  // Test decoded mesh from encoded obj file stays the same.
  test_encoding("bunny_norm.obj");
  test_encoding("cube_att.obj");
  test_encoding("cube_att_partial.obj");
  test_encoding("cube_quads.obj");
  test_encoding("cube_subd.obj");
  test_encoding("extra_vertex.obj");
  test_encoding("multiple_isolated_triangles.obj");
  test_encoding("multiple_tetrahedrons.obj");
  test_encoding("one_face_123.obj");
  test_encoding("one_face_312.obj");
  test_encoding("one_face_321.obj");
  test_encoding("sphere.obj");
  test_encoding("test_nm.obj");
  test_encoding("test_nm_trans.obj");
  test_encoding("test_sphere.obj");
  test_encoding("three_faces_123.obj");
  test_encoding("three_faces_312.obj");
  test_encoding("two_faces_123.obj");
  test_encoding("two_faces_312.obj");
}

TEST_F(MessiahEncoderTest, TestObjOctagonPreserved) {
  // Test verifies that OBJ encoder can reconstruct and encode an octagon.
  // Decode triangulated octagon and an extra attribute for reconstruction.
  std::unique_ptr<draco::Mesh> mesh =
      ReadMeshFromTestFile("octagon_preserved.drc");
  ASSERT_NE(mesh, nullptr);
  ASSERT_EQ(mesh->num_faces(), 6);
  ASSERT_EQ(mesh->NumNamedAttributes(GeometryAttribute::GENERIC), 1);
  ASSERT_NE(mesh->GetMetadata()->GetAttributeMetadataByStringEntry(
                "name", "added_edges"),
            nullptr);

  // Reconstruct octagon and encode it into an OBJ file.
  draco::MessiahEncoder messiah_encoder;
  ASSERT_TRUE(messiah_encoder.EncodeToFile(
      *mesh, draco::GetTestTempFileFullPath("encoded.obj")));

  // Read encoded OBJ file and golden OBJ file contents into buffers.
  std::vector<char> data_encoded;
  std::vector<char> data_golden;
  ASSERT_TRUE(
      ReadFileToBuffer(GetTestTempFileFullPath("encoded.obj"), &data_encoded));
  ASSERT_TRUE(ReadFileToBuffer(GetTestFileFullPath("octagon_preserved.obj"),
                               &data_golden));

  // Check that encoded OBJ file contents are correct.
  ASSERT_EQ(data_encoded.size(), data_golden.size());
  ASSERT_EQ(data_encoded, data_golden);
}

}  // namespace draco
