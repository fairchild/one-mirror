
require 'fileutils'
include FileUtils

LIBS=[
    'src/cloud/common/CloudClient.rb',
    'src/cloud/ec2/lib/views'
]

ECONE_LIBS=[
    'src/cloud/ec2/lib/EC2QueryClient.rb'
]

BIN=[
    'src/cloud/ec2/bin/econe-describe-images',
    'src/cloud/ec2/bin/econe-describe-instances',
    'src/cloud/ec2/bin/econe-register',
    'src/cloud/ec2/bin/econe-run-instances',
    'src/cloud/ec2/bin/econe-terminate-instances',
    'src/cloud/ec2/bin/econe-upload'
]

GEMSPEC='share/gems/econe.gemspec'


rm_rf 'gem_build/econe' 

mkdir_p 'gem_build/econe' 
mkdir_p 'gem_build/econe/lib' 
mkdir_p 'gem_build/econe/lib/econe' 
mkdir_p 'gem_build/econe/bin' 

LIBS.each do |f|
    cp_r f, 'gem_build/econe/lib'
end

ECONE_LIBS.each do |f|
    cp_r f, 'gem_build/econe/lib/econe'
end

BIN.each do |f|
    cp_r f, 'gem_build/econe/bin'
end

cp GEMSPEC, 'gem_build/econe'

cd 'gem_build/econe'

system 'gem build '+File.basename(GEMSPEC)

