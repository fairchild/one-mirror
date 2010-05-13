
require 'fileutils'
include FileUtils

LIBS=[
    'src/client/ruby/client_utilities.rb',
    'src/client/ruby/command_parse.rb',
    'src/oca/ruby/OpenNebula.rb',
    'src/oca/ruby/OpenNebula'
]

BIN=[
    'src/client/ruby/onehost',
    'src/client/ruby/onevm',
    'src/client/ruby/onevnet',
    'src/client/ruby/oneuser'
]

GEMSPEC='share/gems/oca.gemspec'


rm_rf 'gem_build' 

mkdir_p 'gem_build' 
mkdir_p 'gem_build/lib' 
mkdir_p 'gem_build/bin' 

LIBS.each do |f|
    cp_r f, 'gem_build/lib'
end

BIN.each do |f|
    cp_r f, 'gem_build/bin'
end

cp GEMSPEC, 'gem_build'

cd 'gem_build'

system 'gem build '+File.basename(GEMSPEC)

