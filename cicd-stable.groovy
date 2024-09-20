node('linux')
{
  stage ('Poll') {
    checkout([
      $class: 'GitSCM',
      branches: [[name: '*/main']],
      doGenerateSubmoduleConfigurations: false,
      extensions: [],
      userRemoteConfigs: [[url: 'https://github.com/zopencommunity/v8port.git']]])
  }
  stage('Build') {
    build job: 'Port-Pipeline', parameters: [string(name: 'PORT_GITHUB_REPO', value: 'https://github.com/zopencommunity/v8port.git'), string(name: 'PORT_DESCRIPTION', value: 'A port of Google V8 JavaScript engine to z/OS Open Tools project' ), string(name: 'BUILD_LINE', value: 'STABLE') ]
  }
}
