#!/bin/env ruby

# This script is meant to be run one level above the mu2e-artdaq
# repository; provided with a directory (package) name to replace
# "mu2e-artdaq" and a namespace name to replace "demo", it will allow
# experiments to effectively use this stripped-down version of
# mu2e-artdaq as a starting point for their own development

if ! ARGV.length.between?(2,2)
  puts "Usage: ./" + __FILE__.split("/")[-1] + " <dirname> <namespace>"
  exit(1)
end

dirname = ARGV[0]
namespace = ARGV[1]

if ! File.exists?("mu2e-artdaq")
  puts "Should see source directory \"mu2e-artdaq\""
  exit(3)
end

def modify( filename, newdir, newnamespace)

  source = File.read( filename.strip )

  source.gsub!(/artdaq\-demo/, newdir)
  source.gsub!(/demo\:\:/, newnamespace + "::")
  source.gsub!(/namespace(\s+)demo/, "namespace " + newnamespace )
  source.gsub!(/MU2EARTDAQ/, newdir.dup.gsub!(/\-/,"").upcase )
  
  defname = String.new( newdir )
  defname.gsub!("\-", "_")
  source.gsub!("mu2e_artdaq", defname)

  cmd = "rm %s" % [filename]
  `#{cmd}`

  outf = File.open( filename.strip, "w")
  outf.write( source )

end

# First, modify the source files in mu2e-artdaq to reflect the new
# package's directory and namespace

# Searching for every file in the mu2e-artdaq directory less than a
# megabtye is a way of capturing all its source files

results = `find mu2e-artdaq -type f -size -1000k -not -regex ".*\.git.*"`

results.each do |filename|

  puts filename
  filename = filename.strip

  is_executable = nil

  is_executable = "true" if File.stat( filename ).executable?

  if is_executable
    puts "Is executable"
  end
  
  modify( filename, dirname, namespace)

  if is_executable
    cmd = "chmod +x %s" % filename
    `#{cmd}`
  end

end

# Next, actually change the directory names :

cmd = "mv mu2e-artdaq/mu2e-artdaq mu2e-artdaq/" + dirname
`#{cmd}`

cmd = "mv mu2e-artdaq " + dirname
`#{cmd}`

puts "Finished; please check sourcefile comments for references to wikis, repositories, etc. and update the ups/product_deps file to reflect the desired ups version"

