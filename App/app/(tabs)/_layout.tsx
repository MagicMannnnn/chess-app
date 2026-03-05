import { Tabs } from 'expo-router'
import { SymbolView } from 'expo-symbols'

import { theme } from '@/constants/theme'

export default function TabLayout() {
  return (
    <Tabs
      screenOptions={{
        tabBarActiveTintColor: theme.colors.primary,
        headerShown: false,
      }}
    >
      <Tabs.Screen
        name="home"
        options={{
          title: 'Home',
          tabBarIcon: ({ color }) => (
            <SymbolView
              name={{
                ios: 'house.fill',
                android: 'home',
                web: 'home',
              }}
              tintColor={color}
              size={28}
            />
          ),
        }}
      />
      <Tabs.Screen
        name="sandbox"
        options={{
          title: 'Sandbox',
          tabBarIcon: ({ color }) => (
            <SymbolView
              name={{
                ios: 'square.and.pencil',
                android: 'edit',
                web: 'edit',
              }}
              tintColor={color}
              size={28}
            />
          ),
        }}
      />
    </Tabs>
  )
}
