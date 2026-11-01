#include <gtest/gtest.h>
#include "core/geometry/csv_parser.hpp"

using namespace uew::geometry;

static const char* VALID_CSV = R"(node_id,x_m,y_m,z_m,temperature_C
1,0.0,0.0,0.0,100.0
2,0.5,0.0,0.0,150.2
3,1.0,0.0,0.0,200.5
)";

TEST(CSVParser, ValidCSVParsed) {
    auto r = parse_temperature_csv(VALID_CSV);
    EXPECT_TRUE(r.success) << r.error;
    EXPECT_EQ(r.nodes.size(), 3u);
    EXPECT_DOUBLE_EQ(r.nodes[0].temperature_C, 100.0);
    EXPECT_DOUBLE_EQ(r.nodes[1].x, 0.5);
    EXPECT_DOUBLE_EQ(r.t_min, 100.0);
    EXPECT_DOUBLE_EQ(r.t_max, 200.5);
}

TEST(CSVParser, EmptyFileRejected) {
    auto r = parse_temperature_csv("");
    EXPECT_FALSE(r.success);
}

TEST(CSVParser, WrongHeaderRejected) {
    auto r = parse_temperature_csv("id,x,y,z,temp\n1,0,0,0,100\n");
    EXPECT_FALSE(r.success);
}

TEST(CSVParser, MissingColumnRejected) {
    auto r = parse_temperature_csv("node_id,x_m,y_m,z_m,temperature_C\n1,0.0,0.0\n");
    EXPECT_FALSE(r.success);
}

TEST(CSVParser, NonNumericRejected) {
    auto r = parse_temperature_csv("node_id,x_m,y_m,z_m,temperature_C\n1,abc,0.0,0.0,100.0\n");
    EXPECT_FALSE(r.success);
}

TEST(CSVParser, NoDataRowsRejected) {
    auto r = parse_temperature_csv("node_id,x_m,y_m,z_m,temperature_C\n");
    EXPECT_FALSE(r.success);
}
