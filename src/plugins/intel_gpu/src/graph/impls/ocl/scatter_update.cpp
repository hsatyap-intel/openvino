// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "scatter_update_inst.h"
#include "primitive_base.hpp"
#include "impls/implementation_map.hpp"
#include "kernel_selector_helper.h"
#include "scatter_update/scatter_update_kernel_selector.h"
#include "scatter_update/scatter_update_kernel_ref.h"
#include "intel_gpu/runtime/error_handler.hpp"

using namespace cldnn;

namespace cldnn {
namespace ocl {
kernel_selector::scatter_update_axis convert_axis(int64_t axis, const scatter_update_node& arg) {
    auto rank = arg.input(0).get_output_layout().get_rank();
    if (axis < 0) {
        axis += rank;
    }
    auto cldnn_axis = axis;
    if (axis >= 2) {
        auto spatial_axis = axis - 2;
        const size_t default_dims = 4; // Default and minimum number of dimensions is 4
        auto spatial_size = std::max(rank, default_dims) - 2;
        cldnn_axis = spatial_size - spatial_axis - 1 + 2;
    }

    switch (cldnn_axis) {
        case 0: return kernel_selector::scatter_update_axis::BATCH;
        case 1: return kernel_selector::scatter_update_axis::FEATURE;
        case 2: return kernel_selector::scatter_update_axis::X;
        case 3: return kernel_selector::scatter_update_axis::Y;
        case 4: return kernel_selector::scatter_update_axis::Z;
        case 5: return kernel_selector::scatter_update_axis::W;
        default: CLDNN_ERROR_MESSAGE(arg.id(), "Unsupported Axis");
    }
    return kernel_selector::scatter_update_axis::X;
}

struct scatter_update_impl : typed_primitive_impl_ocl<scatter_update> {
    using parent = typed_primitive_impl_ocl<scatter_update>;
    using parent::parent;

    std::unique_ptr<primitive_impl> clone() const override {
        return make_unique<scatter_update_impl>(*this);
    }

public:
    static primitive_impl* create(const scatter_update_node& arg, std::shared_ptr<kernel_impl_params> impl_param) {
        auto scatter_update_params = get_default_params<kernel_selector::scatter_update_params>(*impl_param);
        auto scatter_update_optional_params =
            get_default_optional_params<kernel_selector::scatter_update_optional_params>(arg.get_program());

        scatter_update_params.axis = convert_axis(arg.get_primitive()->axis, arg);

        scatter_update_params.inputs.push_back(convert_data_tensor(impl_param->input_layouts[1]));
        scatter_update_params.inputs.push_back(convert_data_tensor(impl_param->input_layouts[2]));

        auto& kernel_selector = kernel_selector::scatter_update_kernel_selector::Instance();
        auto best_kernels = kernel_selector.GetBestKernels(scatter_update_params, scatter_update_optional_params);

        CLDNN_ERROR_BOOL(arg.id(),
                         "Best_kernel.empty()",
                         best_kernels.empty(),
                         "Cannot find a proper kernel with this arguments");

        auto scatter_update = new scatter_update_impl(arg, best_kernels[0]);

        return scatter_update;
    }
};

namespace detail {

attach_scatter_update_impl::attach_scatter_update_impl() {
    implementation_map<scatter_update>::add(impl_types::ocl, scatter_update_impl::create, {
        std::make_tuple(data_types::f32, format::bfyx),
        std::make_tuple(data_types::f16, format::bfyx),
        std::make_tuple(data_types::i32, format::bfyx),
        std::make_tuple(data_types::f32, format::bfzyx),
        std::make_tuple(data_types::f16, format::bfzyx),
        std::make_tuple(data_types::i32, format::bfzyx),
        std::make_tuple(data_types::f32, format::bfwzyx),
        std::make_tuple(data_types::f16, format::bfwzyx),
        std::make_tuple(data_types::i32, format::bfwzyx),
    });
}

}  // namespace detail
}  // namespace ocl
}  // namespace cldnn
