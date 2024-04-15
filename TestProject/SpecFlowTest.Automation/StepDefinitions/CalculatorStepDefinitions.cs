using System.Diagnostics;
using System.Net.Http.Json;
using System.Text.Json.Nodes;
using SpecFlow.Internal.Json;

namespace SpecFlowTest.Automation.StepDefinitions
{
    [Binding]
    public sealed class CalculatorStepDefinitions
    {
        // For additional details on SpecFlow step definitions see https://go.specflow.org/doc-stepdef
        
        class UnrealSpecFlowPipe
        {
            private Process _unrealProcess = new Process();
            private HttpClient _client = new HttpClient();
            private int _httpPort = 9876;
            private string _host = "localhost";
            private string _protocol = "http";

            public void StartProcess()
            {
                _unrealProcess.StartInfo.FileName = @"cmd.exe";
                //unrealProcess.StartInfo.Arguments = @"/K Engine\Build\BatchFiles\RunUAT.bat RunUnreal -Project=D:\LyraStarterGame\LyraStarterGame.uproject -Build=Engine\Binaries\Win64 -Configuration=DebugGame -Editor -Test=SpecFlow";
                _unrealProcess.StartInfo.Arguments = @"/K Engine\Binaries\Win64\UnrealEditor.exe -Project=D:\LyraStarterGame\LyraStarterGame.uproject -Log -gauntlet=SpecFlow -gauntlet.heartbeatperiod=30 -game";
                _unrealProcess.StartInfo.UseShellExecute = true;
                _unrealProcess.StartInfo.RedirectStandardOutput = false;
                _unrealProcess.StartInfo.CreateNoWindow = false;
                _unrealProcess.StartInfo.WindowStyle = ProcessWindowStyle.Normal;
                _unrealProcess.StartInfo.WorkingDirectory = @"D:\UE_5.3";
                _unrealProcess.Start();
            }

            public Task WaitForExitAsync()
            {
                return _unrealProcess.WaitForExitAsync();
            }

            public async Task<JsonNode?> SendCommandAsync(string specFlowStep, JsonContent? content = null)
            {
                UriBuilder uriBuilder = new UriBuilder(_protocol, _host, _httpPort, "SpecFlow/Steps/" + specFlowStep);
                HttpResponseMessage response = await _client.PostAsync(uriBuilder.Uri, content);
                response.EnsureSuccessStatusCode();
                string jsonString = await response.Content.ReadAsStringAsync();
                return JsonNode.Parse(jsonString);
            }

            public JsonNode? SendCommand(string specFlowStep, JsonContent? content = null)
            {
                Task<JsonNode?> task = SendCommandAsync(specFlowStep, content);
                task.Wait();
                return task.Result;
            }
        }
        
        private HttpClient client = new HttpClient();
        private UnrealSpecFlowPipe _unrealPipe = new UnrealSpecFlowPipe();

        [Given("start an unreal editor instance")]
        public void StartAnUnrealEditorInstance()
        {
            _unrealPipe.StartProcess();
        }

        
        [Then("wait until spec flow plugin is initialized")]
        public void WaitUntilSpecFlowPluginIsInitialized()
        {
            var values = new Dictionary<string, string>
            {
                {"Command", "Test"}
            };
            var content = JsonContent.Create(values);
            content.LoadIntoBufferAsync().Wait();
            JsonNode? resultJson = _unrealPipe.SendCommand("IsSpecFLowPluginInitialized");
        }
        
        [Given("wait (.*) seconds")]
        public void WaitXSeconds(int seconds)
        {
            Task.Delay(seconds * 1000).Wait();
        }

        [When("execute the unreal command \"(.*)\"")]
        public void ExecuteTheUnrealCommand(string command)
        {
            var values = new Dictionary<string, string>
            {
                {"Command", command}
            };

            var content = JsonContent.Create(values);
            content.LoadIntoBufferAsync().Wait();
            JsonNode? resultJson = _unrealPipe.SendCommand("ExecuteUnrealCommand", content);
        }
        
        [Then("wait until unreal process ends")]
        public Task ThenWaitUnrealProcessEnd()
        {
            return _unrealPipe.WaitForExitAsync();
        }
    }
}
