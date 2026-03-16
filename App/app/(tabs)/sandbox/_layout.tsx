import { Stack, useRouter } from 'expo-router'
import { SymbolView } from 'expo-symbols'
import { Pressable } from 'react-native'

import { theme } from '@/constants/theme'

export default function SandboxStack() {
  const router = useRouter()

  return (
    <Stack
      screenOptions={{
        headerLeft: () => (
          <Pressable onPress={() => router.push('/settings')} style={{ marginLeft: 15 }}>
            {({ pressed }) => (
              <SymbolView
                name={{ ios: 'gearshape', android: 'settings', web: 'settings' }}
                size={24}
                tintColor={theme.colors.primary}
                style={{ opacity: pressed ? 0.5 : 1 }}
              />
            )}
          </Pressable>
        ),
      }}
    >
      <Stack.Screen name="index" options={{ title: 'Sandbox' }} />
      <Stack.Screen name="performance-test" options={{ title: 'Performance Test' }} />
    </Stack>
  )
}
