/* Copyright (c) 200, 2008 by CodeSourcery.  All rights reserved.

   This file is available for license from CodeSourcery, Inc. under the terms
   of a commercial license and under the GPL.  It is not part of the VSIPL++
   reference implementation and is not available under the BSD license.
*/
/// Description: VSIPL++ implementation of HPCS Challenge Benchmarks 
///              Scalable Synthetic Compact Applications - 
///              SSCA #3: Sensor Processing and Knowledge Formation

#include <vsip/initfin.hpp>
#include <vsip/math.hpp>
#include <vsip/signal.hpp>
#include <vsip_csl/memory_pool.hpp>
#include "ssar.hpp"
#include "kernel1.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

// Print usage information.
// If an error message is given, print that and exit with -1, otherwise with 0.
void ssar_options::usage(std::string const &prog_name,
			 std::string const &error = std::string())
{
  std::ostream &os = error.empty() ? std::cout : std::cerr;
  if (!error.empty()) os << error << std::endl;
  os << "Usage: " << prog_name 
     << " [-l|--loop <LOOP>] [-o|--output <FILE>] [--swap|--noswap] <data dir> " 
     << std::endl;
  if (error.empty()) std::exit(0);
  else std::exit(-1);
}


ssar_options::ssar_options(int argc, char **argv)
  : loop(1),
#if _BIG_ENDIAN
    swap_bytes(true)
#else
    swap_bytes(false)
#endif
{
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (arg == "-o" || arg == "--output")
    {
      if (++i == argc) usage(argv[0], "no output argument given");
      else output = argv[i];
    }
    else if (arg == "-l" || arg == "-loop" || arg == "--loop")
    {
      if (++i == argc) usage(argv[0], "no loop argument given");
      else
      {
	std::istringstream iss(argv[i]);
	iss >> loop;
	if (!iss) usage(argv[0], "loop argument not an integer");
      }
    }
    else if (arg == "-swap" || arg == "--swap") swap_bytes = true;
    else if (arg == "-noswap" || arg == "--noswap") swap_bytes = false;
    else if (arg[0] != '-')
    {
      if (data_dir.empty()) data_dir = arg;
      else usage(argv[0], "Invalid non-option argument");
    }
    else usage(argv[0], "Unknown option");
  }

  if (data_dir.empty()) usage(argv[0], "No data dir given");

  data_dir += '/';
  sar_dimensions = data_dir + "dims.txt";
  raw_sar_data = data_dir + "sar.view";
  fast_time_filter = data_dir + "ftfilt.view";
  slow_time_wavenumber = data_dir + "k.view";
  slow_time_compressed_aperture_position = data_dir + "uc.view";
  slow_time_aperture_position = data_dir + "u.view";
  slow_time_spatial_frequency = data_dir + "ku.view";

  if (output.empty()) output = data_dir + "image.view";

  std::ifstream ifs(sar_dimensions.c_str());
  if (!ifs) usage(argv[0], "unable to open dimension data file");
  else
  {
    ifs >> scale >> n >> mc >> m;
    if (!ifs) usage(argv[0], "Error reading dimension data");
  }
}

int
main(int argc, char** argv)
{
  using vsip_csl::load_view_as;
  using vsip_csl::save_view_as;

  vsip::vsipl init(argc, argv);

  Local_map huge_map;

  ssar_options opt(argc, argv);

  typedef SSAR_BASE_TYPE T;

#if VSIP_IMPL_ENABLE_HUGE_PAGE_POOL && VSIP_IMPL_CBE_SDK
  vsip_csl::set_pool(huge_map, new vsip_csl::Huge_page_pool("/huge/benchmark.bin", 20));
#endif

  // Setup for Stage 1, Kernel 1 
  vsip::impl::profile::Acc_timer t0;
  t0.start();
  Kernel1<T> k1(opt, huge_map); 
  t0.stop();
  std::cout << "setup:   " << t0.delta() << " (s)" << std::endl;

  // Retrieve the raw radar image data from disk.  This Data I/O 
  // component is currently untimed.
  Kernel1<T>::complex_matrix_type s_raw(opt.n, opt.mc, huge_map);
  load_view_as<complex<float>, 
    Kernel1<T>::complex_matrix_type>(opt.raw_sar_data.c_str(), s_raw, opt.swap_bytes);

  // Resolve the image.  This Computation component is timed.
  Kernel1<T>::real_matrix_type 
    image(k1.output_size(0), k1.output_size(1), huge_map);

  vsip::impl::profile::Acc_timer t1;
  vsip::Vector<double> process_time(opt.loop);
  for (unsigned int l = 0; l < opt.loop; ++l)
  {
    t1.start();
    k1.process_image(s_raw, image);
    t1.stop();
    process_time.put(l, t1.delta());
  }

  // Display statistics
  if (opt.loop > 0)
  {
    Index<1> idx;
    double mean = vsip::meanval(process_time);
    std::cout << "loops:   " << opt.loop << std::endl;
    std::cout << "mean:    " << mean << std::endl;
    std::cout << "min:     " << vsip::minval(process_time, idx) << std::endl;
    std::cout << "max:     " << vsip::maxval(process_time, idx) << std::endl;
    std::cout << "std-dev: " << sqrt(vsip::meansqval(process_time - mean)) << std::endl;
  }

  // Store the image on disk for later processing (not timed).
  save_view_as<float>(opt.output.c_str(), image, opt.swap_bytes); 
}
