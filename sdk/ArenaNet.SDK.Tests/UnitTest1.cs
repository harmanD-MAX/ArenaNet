using NUnit.Framework;
using ArenaNet.SDK;
using System.Threading.Tasks;

namespace ArenaNet.SDK.Tests
{
    public class Tests
    {
        [SetUp]
        public void Setup()
        {
        }

        [Test]
        public void TestClientInstantiation()
        {
            using var client = new ArenaNetClient("127.0.0.1", 8080);
            Assert.That(client, Is.Not.Null);
            Assert.That(client.Auth, Is.Not.Null);
            Assert.That(client.Lobby, Is.Not.Null);
            Assert.That(client.Matchmaking, Is.Not.Null);
            Assert.That(client.Chat, Is.Not.Null);
            Assert.That(client.Leaderboard, Is.Not.Null);
            Assert.That(client.Social, Is.Not.Null);
            Assert.That(client.IsConnected, Is.False);
        }

        [Test]
        public void TestClientConnectionFailureGraceful()
        {
            using var client = new ArenaNetClient("127.0.0.1", 9999); // Invalid port
            Assert.ThrowsAsync<System.Exception>(async () => await client.ConnectAsync());
        }
    }
}
